# Batch Rendering System 解耦设计方案

**文档版本**: v1.0  
**最后更新**: 2025-12-03  
**状态**: 待实施（后续优化）  
**优先级**: 中（在 DX12 稳定后考虑）

---

## 1. 当前问题分析

### 1.1 紧密耦合 DX12

当前 `UIGraphicsSystem` 中的批渲染系统与 DirectX 12 紧密耦合：

- **BatchData 结构体** 包含 DX12 专有类型：
  ```cpp
  DirectX::BasicEffect* _pEffect;
  D3D12_GPU_DESCRIPTOR_HANDLE _srvDescriptor;
  D3D12_GPU_DESCRIPTOR_HANDLE _samplerDescriptor;
  D3D_PRIMITIVE_TOPOLOGY _topology;
  ```

- **RegisterBatchData 接口** 接受 DX12 类型：
  ```cpp
  void RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY topology, ...);
  std::unique_ptr<DirectX::BasicEffect>& effect, ...);
  ```

- **执行逻辑混入 DX12 代码**
  - `ExecuteColorBatch/ExecuteTextureBatch` 在 `UIGraphicsSystem` 中实现
  - 充满 `ID3D12GraphicsCommandList`、`D3D12_GPU_DESCRIPTOR_HANDLE` 等 DX12 细节

### 1.2 逻辑分散

- 批渲染的注册逻辑 → `UIGraphicsSystem`
- 批渲染的执行逻辑 → `UIGraphicsSystem`
- 后端细节（DX12） → 混在 `UIGraphicsSystem` 中
- **结果**: 业务逻辑和图形 API 细节无法分离

### 1.3 扩展性差

若要支持其他图形 API（OpenGL/Vulkan）：
- 需要大规模修改 `UIGraphicsSystem`
- `BatchData` 结构体无法复用
- 每个后端都要维护一套完整的执行逻辑
- 代码重复和维护成本高

### 1.4 逻辑完整性疑虑

**用户疑虑**: 接口分离后会破坏逻辑的完整性

**分析**:
- 这是合理的顾虑，因为批渲染逻辑确实会分散到两个类中
- 但通过合理的设计和文档，可以保持逻辑的清晰性
- 参见第 5 节 "关于逻辑完整性"

---

## 2. 解耦目标

- ✅ `UIGraphicsSystem` **完全无 DX12 依赖**
  - 只知道通用类型（void*、int、RECT 等）
  - 不包含 #include <d3d12.h>

- ✅ `BatchData` **使用后端无关的类型**
  - 使用 `void*` 替代 DX12 专有类型
  - 可被不同后端复用

- ✅ **所有 DX12 执行逻辑移到 `UIGraphicsDeviceDX12`**
  - 保持 DX12 细节在后端层
  - 清晰的职责边界

- ✅ **保持逻辑的清晰性和完整性**
  - 通过合理的文档和接口设计
  - 批渲染流程仍能被理解和维护

---

## 3. 设计方案

### 3.1 BatchData 结构（后端无关）

**位置**: `UIDXFoundation.h` 中 `UIGraphicsSystem` 内

```cpp
private:
    struct BatchData {
        // ========== Keys (used for batching) ==========
        int batchID;                    // 1: color batch (p_batch)
                                        // 2: texture batch (p_batchTexture)
        int renderLevel;                // rendering level (smaller renders first)
        void* pEffect;                  // opaque effect pointer (backend-specific)
        RECT clipRect;                  // clipping rectangle
        void* srvDescriptor;            // opaque SRV descriptor (for texture batch)
        void* samplerDescriptor;        // opaque sampler descriptor (for texture batch)
        UCHAR alpha;                    // transparency (for texture batch)
        UICameraBase* pCamera;          // camera pointer (should abstract in future)

        // ========== Data ==========
        int topology;                   // 0=PointList, 1=LineList, 2=TriangleList
        std::vector<std::vector<uint16_t>> indices;
        std::vector<std::vector<DirectX::VertexPositionColor>> colorVertices;      // for batch 1
        std::vector<std::vector<DirectX::VertexPositionTexture>> textureVertices;  // for batch 2

        // Helper function
        bool IsSameKey(const BatchData& other) const {
            if (batchID != other.batchID || 
                renderLevel != other.renderLevel ||
                pEffect != other.pEffect || 
                memcmp(&clipRect, &other.clipRect, sizeof(RECT)) != 0 ||
                pCamera != other.pCamera) {
                return false;
            }
            
            if (batchID == 2) {  // texture batch
                if (srvDescriptor != other.srvDescriptor || 
                    samplerDescriptor != other.samplerDescriptor ||
                    alpha != other.alpha) {
                    return false;
                }
            }
            
            return true;
        }
    };
```

**关键改动**:
- `void* pEffect` 替代 `DirectX::BasicEffect*`
- `void* srvDescriptor` 替代 `D3D12_GPU_DESCRIPTOR_HANDLE`
- `int topology` 替代 `D3D_PRIMITIVE_TOPOLOGY` 枚举

### 3.2 UIGraphicsSystem 接口（后端无关）

**位置**: `UIDXFoundation.h` 中 `UIGraphicsSystem` 类

```cpp
private:
    // ========== Batch Registration (Backend-Agnostic) ==========
    void RegisterBatchData(int topology,
                           const std::vector<DirectX::VertexPositionColor>& vertices,
                           const std::vector<uint16_t>& indices,
                           void* pEffect,              // opaque pointer
                           const RECT& clipRect,
                           UICameraBase* pCamera,
                           int renderLevel = 0);

    void RegisterBatchTextureData(int topology,
                                  const std::vector<DirectX::VertexPositionTexture>& vertices,
                                  const std::vector<uint16_t>& indices,
                                  void* pEffect,              // opaque pointer
                                  void* srvDescriptor,        // opaque pointer
                                  void* samplerDescriptor,    // opaque pointer
                                  UCHAR alpha,
                                  const RECT& clipRect,
                                  UICameraBase* pCamera,
                                  int renderLevel = 0);

    // ========== Batch Execution (Delegate to Backend) ==========
    void ExecuteAllBatches() {
        if (_batchDataList.empty()) return;

        // Sort batches by render level
        std::sort(_batchDataList.begin(), _batchDataList.end(),
            [](const BatchData& a, const BatchData& b) { 
                return a.renderLevel < b.renderLevel; 
            });

        // Delegate execution to backend device (ZERO DX12 code here!)
        _graphicsDevice->ExecuteBatchData(_batchDataList);

        ClearAllBatches();
    }

    void ClearAllBatches();

    // ========== Batch Data ==========
    std::vector<BatchData> _batchDataList;
    UICameraBase* _pCurrentCamera = nullptr;
    void* _pCurrentEffect = nullptr;  // opaque
```

**关键特点**:
- 接口参数使用 `void*` 而非 DX12 类型
- `ExecuteAllBatches` 只负责排序和代理
- 无任何 DX12 执行逻辑
- 无需 `#include <d3d12.h>`

### 3.3 UIGraphicsDeviceDX12 执行接口（后端特定）

**位置**: `UIDXFoundation.h` 中 `UIGraphicsDeviceDX12` 类

```cpp
public:
    // ========== Backend-Specific Batch Execution ==========
    void ExecuteBatchData(const std::vector<UIGraphicsSystem::BatchData>& batches);

private:
    // ========== DX12-Specific Execution Helpers ==========
    void ExecuteColorBatch(const UIGraphicsSystem::BatchData& batch);
    void ExecuteTextureBatch(const UIGraphicsSystem::BatchData& batch);
    void UpdateBatchState(const UIGraphicsSystem::BatchData& batch);
```

**说明**:
- 所有 DX12 执行逻辑移到这里
- 参数仍为通用 `BatchData`，但内部转换为 DX12 类型
- 完全隐藏 DirectX 细节

### 3.4 调用流程

```
UIGraphicsSystem::Draw2DPoint()
    ↓
RegisterBatchData(int topology, void* pEffect, ...)  // 无 DX12
    ↓
UIGraphicsSystem::ExecuteAllBatches()  // 只排序和代理
    ↓
UIGraphicsDeviceDX12::ExecuteBatchData(batches)  // DX12 执行
    ↓
ExecuteColorBatch() / ExecuteTextureBatch()  // DX12 细节
```

---

## 4. 实现示例

### 4.1 UIGraphicsSystem 调用示例

**位置**: `UIDXFoundation.cpp`

```cpp
void UIGraphicsSystem::Draw2DPoint(const DirectX::XMFLOAT2& point, float z, 
                                   const UIColor& color, float pointSize, int renderLevel) {
    // ...create vertices...
    
    // Get effect as opaque void* (no DX12 knowledge here!)
    auto* effect = _graphicsDevice->_effects.p_pointEffect2D.get();
    
    RegisterBatchData(0,  // topology: 0=PointList
                      vertices, indices,
                      static_cast<void*>(effect),  // cast to opaque pointer
                      _currentClipRect,
                      nullptr,
                      renderLevel);
}

void UIGraphicsSystem::Draw2DImage(size_t textureIndex, RECT srcRect, 
                                   DirectX::XMFLOAT2 dstStart, DirectX::XMFLOAT2 dstEnd, 
                                   float z, UCHAR alpha, int renderLevel) {
    // ...create texture vertices...
    
    auto* effect = _graphicsDevice->_effects.p_triangleTexturedEffect2D.get();
    auto& texResource = _graphicsDevice->_textures._textureResources[textureIndex];
    
    // Store descriptor handles as opaque void*
    void* srvHandle = static_cast<void*>(&texResource._gpuDescriptor);
    void* samplerHandle = nullptr;  // get from states
    
    RegisterBatchTextureData(2,  // topology: 2=TriangleList
                             textureVertices, indices,
                             static_cast<void*>(effect),
                             srvHandle,
                             samplerHandle,
                             alpha,
                             _currentClipRect,
                             nullptr,
                             renderLevel);
}
```

**关键点**:
- 使用 `static_cast<void*>(...)` 转换指针
- 无需包含任何 DX12 头文件
- 逻辑清晰，只关心批数据收集

### 4.2 UIGraphicsDeviceDX12 执行示例

**位置**: `UIDXFoundation.cpp`

```cpp
void UIGraphicsDeviceDX12::ExecuteBatchData(const std::vector<UIGraphicsSystem::BatchData>& batches) {
    auto* cmdList = _commandList.Get();
    if (!cmdList) return;

    for (const auto& batch : batches) {
        UpdateBatchState(batch);
        
        if (batch.batchID == 1) {
            ExecuteColorBatch(batch);
        } else if (batch.batchID == 2) {
            ExecuteTextureBatch(batch);
        }
    }
}

void UIGraphicsDeviceDX12::ExecuteColorBatch(const UIGraphicsSystem::BatchData& batch) {
    auto* cmdList = _commandList.Get();
    
    // Cast opaque pointers back to DX12 types (ONLY HERE!)
    auto* effect = static_cast<DirectX::BasicEffect*>(batch.pEffect);
    if (!effect) return;

    // Set scissor rect
    D3D12_RECT scissorRect = {
        batch.clipRect.left, batch.clipRect.top,
        batch.clipRect.right, batch.clipRect.bottom
    };
    cmdList->RSSetScissorRects(1, &scissorRect);

    // Apply effect
    effect->Apply(cmdList);

    // Map topology (0->POINTLIST, 1->LINELIST, 2->TRIANGLELIST)
    D3D_PRIMITIVE_TOPOLOGY topology = static_cast<D3D_PRIMITIVE_TOPOLOGY>(batch.topology + 1);

    // Render all vertex/index buffers
    for (size_t i = 0; i < batch._colorVertices.size(); ++i) {
        if (batch._colorVertices[i].empty() || batch._indices[i].empty()) continue;

        p_batch->Begin(cmdList);
        p_batch->DrawIndexed(topology,
                             batch._indices[i].data(),
                             static_cast<uint32_t>(batch._indices[i].size()),
                             batch._colorVertices[i].data(),
                             static_cast<uint32_t>(batch._colorVertices[i].size()));
        p_batch->End();
    }
}

void UIGraphicsDeviceDX12::ExecuteTextureBatch(const UIGraphicsSystem::BatchData& batch) {
    auto* cmdList = _commandList.Get();
    
    // Cast opaque pointers back to DX12 types
    auto* effect = static_cast<DirectX::BasicEffect*>(batch.pEffect);
    auto* srvDesc = static_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(batch.srvDescriptor);
    auto* samplerDesc = static_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(batch.samplerDescriptor);
    
    if (!effect || !srvDesc) return;

    // Set scissor rect
    D3D12_RECT scissorRect = {
        batch.clipRect.left, batch.clipRect.top,
        batch.clipRect.right, batch.clipRect.bottom
    };
    cmdList->RSSetScissorRects(1, &scissorRect);

    // Set texture descriptor
    effect->SetTexture(*srvDesc, p_states->LinearClamp());
    
    // Apply effect with alpha
    if (batch.alpha < 255) {
        effect->SetAlpha(batch.alpha / 255.0f);
    }
    effect->Apply(cmdList);

    // Map topology
    D3D_PRIMITIVE_TOPOLOGY topology = static_cast<D3D_PRIMITIVE_TOPOLOGY>(batch.topology + 1);

    // Render all vertex/index buffers
    for (size_t i = 0; i < batch._textureVertices.size(); ++i) {
        if (batch._textureVertices[i].empty() || batch._indices[i].empty()) continue;

        p_batchTexture->Begin(cmdList);
        p_batchTexture->DrawIndexed(topology,
                                    batch._indices[i].data(),
                                    static_cast<uint32_t>(batch._indices[i].size()),
                                    batch._textureVertices[i].data(),
                                    static_cast<uint32_t>(batch._textureVertices[i].size()));
        p_batchTexture->End();
    }
}

void UIGraphicsDeviceDX12::UpdateBatchState(const UIGraphicsSystem::BatchData& batch) {
    auto* effect = static_cast<DirectX::BasicEffect*>(batch.pEffect);
    if (!effect) return;

    // Update camera matrices if present
    if (batch.pCamera) {
        // effect->SetView(batch.pCamera->GetViewMatrix());
        // effect->SetProjection(batch.pCamera->GetProjectionMatrix());
    }
}
```

**关键点**:
- 所有转换都在这里进行
- 所有 DX12 API 调用都在这里
- 逻辑完整，易于调试和维护

---

## 5. 关于"逻辑完整性"的疑虑

### 5.1 问题

**用户疑虑**: 接口分离后，批渲染逻辑分散在两个类中，会破坏逻辑的完整性。

**可能的表现**:
- 代码难以理解整个批渲染流程
- 维护时容易遗漏某些步骤
- 跨类依赖关系不明确

### 5.2 分析

**实际上**:
- 这种分离是合理的架构设计（关注点分离）
- 业务逻辑（批数据收集）与技术实现（DX12 渲染）本应分离
- 通过合理的设计和文档，可以保持逻辑的清晰性

**类比**:
```
网络编程中：
- 应用层：业务逻辑（socket write）
- 传输层：具体实现（TCP/UDP）

同样的原理应用到图形系统：
- 系统层（UIGraphicsSystem）：业务逻辑（批收集）
- 后端层（UIGraphicsDeviceDX12）：具体实现（DX12 渲染）
```

### 5.3 解决方案（三选一）

#### 方案 A：接口隔离（推荐，长期友好）

**优点**:
- ✅ 后端彻底解耦
- ✅ 支持多后端
- ✅ 职责清晰

**缺点**:
- ❌ 逻辑分散到两个类
- ❌ 需要更多文档

**实施**:
- 如本文档所示
- 通过清晰的文档和注释维持逻辑关联
- 建议在类注释中加入"批渲染流程图"

#### 方案 B：抽象中间层（折中，短期实用）

**思路**:
- 在 `UIGraphicsSystem` 中保留执行接口
- 但接受 `void*` 参数（后端无关）
- 执行时判断 `_graphicsDevice->GetBackendName()` 来调用不同后端

**优点**:
- ✅ 逻辑相对完整（都在 UIGraphicsSystem）
- ✅ 不需要改动太多代码

**缺点**:
- ❌ UIGraphicsSystem 中有 if-else 判断 backend
- ❌ 若新增 backend，需要修改 UIGraphicsSystem

**示例**:
```cpp
void UIGraphicsSystem::ExecuteAllBatches() {
    // ...sort...
    
    if (_graphicsDevice->GetBackendName() == "DirectX 12") {
        ExecuteAllBatchesDX12();
    } else if (_graphicsDevice->GetBackendName() == "OpenGL") {
        ExecuteAllBatchesOpenGL();
    }
}
```

#### 方案 C：延迟实施（现状，保险）

**思路**:
- 暂时不改动执行逻辑
- 继续使用 DX12 类型
- 未来若需要多后端支持再做解耦

**优点**:
- ✅ 不需要改动现有代码
- ✅ 风险最小

**缺点**:
- ❌ 后续改动成本更高
- ❌ 技术债不断积累

### 5.4 建议

| 时间段 | 方案 | 说明 |
|--------|------|------|
| **短期**（当前） | C | 保持现状，但在代码中预留接口（使用 void* 参数）|
| **中期**（有多后端需求时） | B | 抽象中间层，保持逻辑在 UIGraphicsSystem |
| **长期**（架构成熟后） | A | 完全接口隔离，通过文档维持逻辑关联 |

---

## 6. 当前代码对比

### 6.1 改动前

```cpp
// UIGraphicsSystem
void RegisterBatchData(D3D_PRIMITIVE_TOPOLOGY topology,
                       ...,
                       std::unique_ptr<DirectX::BasicEffect>& effect,  // DX12!
                       ...);

void ExecuteColorBatch(const BatchData& batch, ID3D12GraphicsCommandList* commandList) {  // DX12!
    // ... 大量 DX12 代码 ...
}
```

### 6.2 改动后

```cpp
// UIGraphicsSystem
void RegisterBatchData(int topology,
                       ...,
                       void* pEffect,  // 无 DX12
                       ...);

// UIGraphicsDeviceDX12
void ExecuteColorBatch(const UIGraphicsSystem::BatchData& batch) {  // 只在这里 DX12
    auto* effect = static_cast<DirectX::BasicEffect*>(batch.pEffect);
    // ... DX12 代码 ...
}
```

---

## 7. 后续实现清单

### 7.1 代码改动

- [ ] 修改 `BatchData` 结构
  - [ ] `void* pEffect` 替代 `DirectX::BasicEffect*`
  - [ ] `void* srvDescriptor` 替代 `D3D12_GPU_DESCRIPTOR_HANDLE`
  - [ ] `int topology` 替代 `D3D_PRIMITIVE_TOPOLOGY`

- [ ] 更新 `RegisterBatchData/RegisterBatchTextureData` 签名
  - [ ] 参数改为 `void*` 类型
  - [ ] 移除 `std::unique_ptr<DirectX::BasicEffect>&`
  - [ ] 移除 `D3D_PRIMITIVE_TOPOLOGY` 参数

- [ ] 将执行逻辑移到 `UIGraphicsDeviceDX12`
  - [ ] 移动 `ExecuteColorBatch` 方法
  - [ ] 移动 `ExecuteTextureBatch` 方法
  - [ ] 移动 `UpdateBatchState` 方法

- [ ] 更新所有调用点
  - [ ] 改为 `static_cast<void*>(effect.get())`
  - [ ] 改为 `int topology` 参数（0/1/2）
  - [ ] 改为 `static_cast<void*>(&descriptor)`

- [ ] 验证编译和运行
  - [ ] 无编译错误
  - [ ] 渲染结果与之前一致

### 7.2 文档补充

- [ ] 在 `UIGraphicsSystem` 类注释中添加"批渲染流程图"
- [ ] 在 `UIGraphicsDeviceDX12::ExecuteBatchData` 注释中说明转换逻辑
- [ ] 更新本文档链接到代码

### 7.3 测试验证

- [ ] 单元测试：批数据排序正确性
- [ ] 集成测试：不同场景下的渲染结果
- [ ] 性能测试：确保无性能下降

---

## 8. 风险评估

| 风险 | 等级 | 缓解方案 |
|------|------|----------|
| void* 转换不安全 | 中 | 添加类型检查和断言 |
| 性能下降 | 低 | 转换发生在执行阶段，无额外开销 |
| 逻辑复杂度上升 | 中 | 充分的文档和注释 |
| 与现有代码冲突 | 低 | 改动仅涉及内部实现，接口兼容 |

---

## 9. 相关文件

- **设计实现相关**
  - `UIDXFoundation.h` - 主要改动点
  - `UIDXFoundation.cpp` - 实现改动

- **参考文档**
  - 本文档位置：`AmazeUI/AmazeUI-D12/DOC/BatchRenderingSystem_Decoupling_Design.md`

---

## 10. 术语表

| 术语 | 说明 |
|------|------|
| **BatchData** | 批次数据结构，包含批次的关键参数和顶点数据 |
| **Backend** | 图形 API 后端实现（如 DirectX 12） |
| **Decoupling** | 解耦，减少不同模块之间的依赖关系 |
| **HAL** | Hardware Abstraction Layer，硬件抽象层 |
| **Opaque Pointer** | 不透明指针，类型为 void*，具体类型由后端定义 |

---

**文档维护者**: AmazeUI Graphics Team  
**最后更新**: 2025-12-03  
**下次审查**: 实施后一周
