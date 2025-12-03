# 虚拟 GPU 接口层设计计划与 PR 流程

## 文档版本

| 版本 | 日期 | 作者 | 说明 |
|------|------|------|------|
| 1.0 | 2025-01-XX | - | 初始版本 |
| 2.0 | 2025-01-XX | - | 基于深度分析重构，明确三阶段策略 |
| 2.1 | 2025-11-30 | - | 更新实施进度，Phase 1 核心文件已完成 |
| 2.2 | 2025-11-30 | - | DX12GraphicsDevice 改用 SingletonPattern 模板基类 |
| 2.3 | 2025-11-30 | - | UIGraphicsFoundation 重构为单例，持有 IGraphicsDevice |

---

## 1. 背景与目标

### 1.1 背景

AmazeUI-D12 当前架构中，DX12 特定类型（ComPtr、ID3D12Device、ID3D12Resource 等）深度耦合在 UI 层代码中。这导致：
- 无法轻松切换到其他图形后端（Vulkan、Metal、WebGPU）
- UI 层代码包含大量平台特定头文件
- 测试和模拟困难

### 1.2 目标

**核心目标**：实现一个真正的虚拟 GPU 接口层
- **底层**：可以接入 DX12 或其他 OS 的底层绘制 API
- **上层**：整个 AmazeUI 上层 UI 不再出现任何 DX12 的特定类型

---

## 2. 技术分析

### 2.1 DirectXMath 使用分析

**分析结论**：DirectXMath 被广泛使用于 UI 层

| 文件 | 使用的类型 | 用途 |
|------|------------|------|
| UIElement.h | XMFLOAT2, XMFLOAT4 | 位置、大小、颜色 |
| UIWindow.h | XMFLOAT2, XMFLOAT4 | 窗口边界、变换 |
| UICamera.h | XMFLOAT3, XMFLOAT4X4 | 视图/投影矩阵 |
| UIModel.h | XMFLOAT3, XMFLOAT4X4 | 3D 模型变换 |
| UIDXFoundation.h | 全部 | 渲染管线 |

**决策**：
- DirectXMath 是**仅头文件库**，不绑定任何 GPU API
- **Phase 1 保留** DirectXMath，通过类型别名封装
- Phase 3 可选择迁移到自定义数学库

### 2.2 BatchRender 分析

**分析结论**：BatchRender 对所有后端都有效

BatchRender 的核心设计是**批处理渲染**：
1. 收集多个绘制项
2. 按类型/状态排序
3. 合并绘制调用
4. 减少状态切换

**多后端适用性**：

| 后端 | 批处理优势 | 说明 |
|------|------------|------|
| DX12 | ✅ 高 | 减少 command list 记录 |
| Vulkan | ✅ 高 | 减少 vkCmdDraw 调用 |
| Metal | ✅ 高 | 减少 encoder 状态切换 |
| WebGPU | ✅ 高 | 减少 JavaScript 开销 |
| OpenGL | ✅ 中 | 减少 glDraw* 调用 |

**决策**：
- **Phase 1**：将 BatchRender 迁移到 DX12 后端
- **Phase 2**：抽象为 IBatchRenderer 接口

### 2.3 DX12 特定类型分析

需要完全隔离到后端的类型：

**COM 智能指针**：
```cpp
ComPtr<ID3D12Device>
ComPtr<ID3D12CommandQueue>
ComPtr<ID3D12GraphicsCommandList>
ComPtr<IDXGISwapChain4>
// ... 其他 ComPtr
```

**DirectXTK12 组件**：
```cpp
std::unique_ptr<BasicEffect>
std::unique_ptr<SpriteBatch>
std::unique_ptr<GraphicsMemory>
std::unique_ptr<PrimitiveBatch<...>>
// ... 其他 DirectXTK 组件
```

**决策**：所有 DX12 类型必须迁移到 DX12GraphicsDevice 中

---

## 3. 三阶段实施策略

### 3.1 最终架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         UI 层                                    │
│  (UIElement, UIWindow, UIWidget 等)                              │
│                         │                                        │
│                         ▼ 只知道                                  │
├─────────────────────────────────────────────────────────────────┤
│            UIGraphicsFoundation (单例)                           │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │   对 UI 层的唯一绘图接口                                   │  │
│  │   ├── Initialize/Shutdown/BeginFrame/EndFrame/Present      │  │
│  │   ├── 2D/3D Drawing APIs                                   │  │
│  │   ├── Texture Management                                   │  │
│  │   ├── FreeType Text (future)                               │  │
│  │   ├── Batch Rendering                                      │  │
│  │   └── OnDeviceLost/OnDeviceRestored                        │  │
│  └───────────────────────────────────────────────────────────┘  │
│                         │ 内部持有                               │
│                         ▼                                        │
├─────────────────────────────────────────────────────────────────┤
│            IGraphicsDevice (抽象接口)                            │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │   虚拟 GPU 接口，可替换后端                                │  │
│  │   ├── Initialize/Shutdown                                 │  │
│  │   ├── BeginFrame/EndFrame/Present                         │  │
│  │   └── HandleWindowResize/WaitForIdle                      │  │
│  └───────────────────────────────────────────────────────────┘  │
│                         ▲                                        │
│                         │ 实现                                   │
├─────────────────────────────────────────────────────────────────┤
│            DX12GraphicsDevice (单例)                             │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │   DX12 后端实现                                           │  │
│  │   ├── UIDeviceResources (DX12 核心资源)                   │  │
│  │   ├── UIEffectManager (Effect 管理)                       │  │
│  │   ├── 所有 ComPtr<ID3D12*> 成员                          │  │
│  │   └── 所有 DirectXTK12 组件                               │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 设备丢失处理流程

```
UIDeviceResources::HandleDeviceLost()
    │
    ├──► UIGraphicsFoundation::OnDeviceLost()
    │        └── 清理批处理、纹理缓存
    │
    ├──► DX12GraphicsDevice::OnDeviceLost()
    │        └── 释放 XTK 资源
    │
    ├──► 释放 DX12 设备资源
    │
    ├──► 重建 DX12 设备资源
    │
    ├──► DX12GraphicsDevice::OnDeviceRestored()
    │        └── 重建 XTK 资源
    │
    └──► UIGraphicsFoundation::OnDeviceRestored()
             └── 重建批处理 batches
```

### 3.3 Phase 实施计划

---

## 4. Phase 1 详细设计

### 4.1 新增文件结构

```
AmazeUI-D12/
├── UI/
│   ├── IGraphicsDevice.h       # ✅ 虚拟GPU接口
│   ├── Types.h                 # ✅ 类型别名
│   ├── DX12GraphicsDevice.h    # ✅ DX12后端 (含UIDeviceResources, UIEffectManager)
│   ├── DX12GraphicsDevice.cpp  # ✅ DX12后端实现
│   ├── UIGraphicsFoundation.h  # ✅ 高层渲染基础 (含UITextureManager)
│   ├── UIGraphicsFoundation.cpp# ✅ 高层渲染实现
│   ├── UIDXFoundation.h        # 原有文件 (待迁移/废弃)
│   └── UIDXFoundation.cpp      # 原有文件 (待迁移/废弃)
```

### 4.2 IGraphicsDevice 接口

```cpp
// Graphics/IGraphicsDevice.h
#pragma once

namespace AmazeUI {

class IGraphicsDevice {
public:
    virtual ~IGraphicsDevice() = default;

    // 生命周期
    virtual bool Initialize(void* windowHandle, int width, int height) = 0;
    virtual void Shutdown() = 0;

    // 帧管理
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    // 窗口管理
    virtual void HandleWindowResize(int width, int height) = 0;
    virtual void WaitForIdle() = 0;

    // 信息查询
    virtual const char* GetBackendName() const = 0;
};

} // namespace AmazeUI
```

### 4.3 Types.h 类型别名

```cpp
// Graphics/Types.h
#pragma once
#include <DirectXMath.h>

namespace AmazeUI {

// 向量类型别名
using Vector2 = DirectX::XMFLOAT2;
using Vector3 = DirectX::XMFLOAT3;
using Vector4 = DirectX::XMFLOAT4;
using Matrix4x4 = DirectX::XMFLOAT4X4;
using Color = DirectX::XMFLOAT4;

// 工具函数
inline Vector2 MakeVector2(float x, float y) { return { x, y }; }
inline Vector3 MakeVector3(float x, float y, float z) { return { x, y, z }; }
inline Vector4 MakeVector4(float x, float y, float z, float w) { return { x, y, z, w }; }
inline Color MakeColor(float r, float g, float b, float a = 1.0f) { return { r, g, b, a }; }

} // namespace AmazeUI
```

### 4.4 DX12GraphicsDevice

```cpp
// Graphics/DX12/DX12GraphicsDevice.h
#pragma once
#include "../IGraphicsDevice.h"

// 所有 DX12 头文件仅在此处包含
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
// ... DirectXTK12 headers

namespace AmazeUI {

class DX12GraphicsDevice : public IGraphicsDevice {
public:
    // IGraphicsDevice 接口实现
    bool Initialize(void* windowHandle, int width, int height) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;
    void HandleWindowResize(int width, int height) override;
    void WaitForIdle() override;
    const char* GetBackendName() const override { return "DirectX 12"; }

    // DX12 特定访问（仅限后端内部使用）
    ID3D12Device* GetDevice() const { return m_device.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }

private:
    // 所有 DX12 成员变量
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
    // ... 其他 DX12 资源

    // DirectXTK12 组件
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::BasicEffect> m_basicEffect;
    // ... 其他 DirectXTK 组件

    // BatchRender (Phase 1 保留在 DX12 后端)
    std::vector<RenderItem> m_renderQueue;
    void FlushBatch();
};

} // namespace AmazeUI
```

### 4.5 UIGraphicsFoundation 重构

```cpp
// UI/UIGraphicsFoundation.h
#pragma once
#include "../Graphics/IGraphicsDevice.h"
#include "../Graphics/Types.h"

namespace AmazeUI {

class UIGraphicsFoundation {
public:
    // 初始化指定后端
    bool Initialize(void* windowHandle, int width, int height, 
                   const char* backendName = "DX12");
    void Shutdown();

    // 委托给 IGraphicsDevice
    void BeginFrame() { m_device->BeginFrame(); }
    void EndFrame() { m_device->EndFrame(); }
    void Present() { m_device->Present(); }

    // 获取设备
    IGraphicsDevice* GetDevice() const { return m_device.get(); }

private:
    std::unique_ptr<IGraphicsDevice> m_device;
};

} // namespace AmazeUI
```

---

## 5. Phase 2 详细设计

### 5.1 IBatchRenderer 接口

```cpp
// Graphics/IBatchRenderer.h
#pragma once
#include "Types.h"

namespace AmazeUI {

struct RenderItem {
    enum class Type { Sprite, Rect, Line, Text, Triangle };
    Type type;
    Vector2 position;
    Vector2 size;
    Color color;
    // ... 其他属性
};

class IBatchRenderer {
public:
    virtual ~IBatchRenderer() = default;

    virtual void Begin() = 0;
    virtual void Submit(const RenderItem& item) = 0;
    virtual void Flush() = 0;
    virtual void End() = 0;

    virtual void SetViewport(float x, float y, float width, float height) = 0;
    virtual void SetScissorRect(float x, float y, float width, float height) = 0;
};

} // namespace AmazeUI
```

### 5.2 资源句柄系统

```cpp
// Graphics/ResourceHandle.h
#pragma once
#include <cstdint>

namespace AmazeUI {

// 不透明资源句柄
struct TextureHandle { uint64_t id = 0; bool IsValid() const { return id != 0; } };
struct BufferHandle { uint64_t id = 0; bool IsValid() const { return id != 0; } };
struct ShaderHandle { uint64_t id = 0; bool IsValid() const { return id != 0; } };

// IResourceManager 接口
class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    virtual TextureHandle CreateTexture(int width, int height, const void* data) = 0;
    virtual void DestroyTexture(TextureHandle handle) = 0;

    virtual BufferHandle CreateBuffer(size_t size, const void* data) = 0;
    virtual void DestroyBuffer(BufferHandle handle) = 0;
};

} // namespace AmazeUI
```

---

## 6. Phase 3 详细设计（可选）

### 6.1 自定义数学库

如果需要完全脱离 DirectXMath，可以实现：

```cpp
// Math/Vector.h
namespace AmazeUI {

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
    // 运算符重载...
};

struct Vector3 {
    float x, y, z;
    // ...
};

struct Vector4 {
    float x, y, z, w;
    // ...
};

struct Matrix4x4 {
    float m[4][4];
    // ...
};

} // namespace AmazeUI
```

**注意**：Phase 3 为可选，DirectXMath 作为仅头文件库，不会阻碍其他后端实现。

---

## 7. 实施步骤

### Step 1: 创建接口文件 ✅ 已完成
- [x] 创建 `UI/IGraphicsDevice.h` - 虚拟GPU接口
- [x] 创建 `UI/Types.h` - 类型别名

### Step 2: 创建 DX12 后端 ✅ 已完成
- [x] 创建 `UI/DX12GraphicsDevice.h`
  - [x] `IDeviceNotify` 接口 - 设备丢失/恢复回调
  - [x] `UIDeviceResources` 类 - DX12 核心资源管理
  - [x] `UIEffectManager` 结构 - Effect 管理 (p_* 命名)
  - [x] `DX12GraphicsDevice` 类 - 实现 IGraphicsDevice + IDeviceNotify
- [x] 创建 `UI/DX12GraphicsDevice.cpp`
  - [x] UIDeviceResources 完整实现
  - [x] UIEffectManager::Create/Reset 实现
  - [x] DX12GraphicsDevice 完整实现

### Step 3: 创建 UIGraphicsFoundation ✅ 已完成
- [x] 创建 `UI/UIGraphicsFoundation.h`
  - [x] `UITextureManager` 类 - 纹理资源管理
  - [x] `UIGraphicsFoundation` 类 - 高层渲染基础
  - [x] `BatchData` 结构 - 批处理渲染数据
- [x] 创建 `UI/UIGraphicsFoundation.cpp`
  - [x] UITextureManager 完整实现 (WIC/DDS 加载, colorKey 透明)
  - [x] 2D UI APIs (Draw2DPoint, Draw2DLine, Draw2DRect*, Draw2DImage)
  - [x] 3D UI APIs (Draw3DPoint, Draw3DLine, Draw3DRect*, Draw3DImage)
  - [x] 3D World APIs (Draw3DWorld*)
  - [x] Batch 渲染系统 (RegisterBatch*, ExecuteAllBatches)
  - [x] ClearBatches/FlushBatches (替代 BeginFrame/EndFrame)

### Step 4: 更新依赖 🔄 进行中
- [ ] 更新 UIWIN32APP - 使用新的图形设备
- [ ] 更新 UIWindow
- [ ] 更新 UIElement
- [ ] 更新其他 UI 组件
- [ ] 移除 UIDXFoundation 的直接依赖

### Step 5: 测试验证 ⏳ 待开始
- [ ] 编译测试
- [ ] 运行现有 Demo
- [ ] 验证功能完整性

---

## 8. PR 流程

### 8.1 PR 拆分策略

| PR | 内容 | 预计改动 |
|----|------|----------|
| PR1 | 创建接口文件 | ~100 行新增 |
| PR2 | 创建 DX12GraphicsDevice | ~500 行迁移 |
| PR3 | 重构 UIGraphicsFoundation | ~300 行修改 |
| PR4 | 更新 UI 组件依赖 | ~200 行修改 |
| PR5 | 添加 Types.h 别名使用 | ~150 行修改 |

### 8.2 PR 模板

```markdown
## 概述
[简要说明本 PR 的目的]

## 改动内容
- [ ] 文件变更列表
- [ ] 主要修改说明

## 测试
- [ ] 编译通过
- [ ] Demo 运行正常
- [ ] 无功能退化

## 相关 Issue
- #xxx

## 截图（如适用）
```

---

## 9. 代码审查清单

### 9.1 接口设计
- [ ] 接口方法是否清晰
- [ ] 参数类型是否平台无关
- [ ] 返回值是否合理

### 9.2 类型隔离
- [ ] UI 层是否无 DX12 头文件
- [ ] UI 层是否无 ComPtr
- [ ] UI 层是否无 ID3D12* 类型

### 9.3 兼容性
- [ ] 现有功能是否完整保留
- [ ] API 是否向后兼容

---

## 10. 里程碑

| 阶段 | 目标 | 预计时间 |
|------|------|----------|
| M1 | Phase 1 完成 - 接口抽象 | 2 周 |
| M2 | Phase 2 完成 - 渲染抽象 | 3 周 |
| M3 | Vulkan 后端原型 | 4 周 |
| M4 | Phase 3 完成（可选） | 2 周 |

---

## 11. 关键决策记录

| 决策 | 选择 | 理由 |
|------|------|------|
| DirectXMath 处理 | Phase 1 保留，类型别名封装 | 仅头文件库，不绑定 GPU API |
| BatchRender 处理 | Phase 1 迁移，Phase 2 抽象 | 对所有后端都有效 |
| DX12 类型处理 | 完全隔离到后端 | 核心目标要求 |
| 命名规范 | UIGraphicsFoundation | 反映平台无关性 |
| 单例模式 | 继承 SingletonPattern\<T\> | 统一框架内单例实现方式，与 UICameraUI2D/UIWin32APP 等保持一致 |

---

## 12. 参考资料

- DirectX 12 编程指南
- Vulkan 规范
- 跨平台渲染架构设计
- BGFX 源码分析
