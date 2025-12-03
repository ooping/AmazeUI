# SkinnedEmissiveEffect

自定义 DirectX 12 Effect，支持骨骼动画和自发光（Emissive）效果。

## 特性

### ✨ 核心功能
- **突破骨骼限制**：支持最多 256 根骨骼（DirectXTK12 的 SkinnedEffect 仅支持 72 根）
- **自发光支持**：完整的 Emissive 纹理和颜色支持
- **三点光照系统**：Key Light、Fill Light、Rim Light
- **高性能**：使用 StructuredBuffer 存储骨骼变换矩阵

### 📊 技术突破

| 特性 | DirectXTK SkinnedEffect | SkinnedEmissiveEffect |
|------|------------------------|----------------------|
| 骨骼数量 | ❌ 72 | ✅ **256** |
| Emissive 支持 | ❌ 无 | ✅ **完整支持** |
| 存储方式 | ConstantBuffer (64 KB 限制) | ✅ **StructuredBuffer** (128 MB) |
| 性能 | 高 | ✅ **相当或更高** |

## 文件结构

```
AmazeUI/AmazeUI-D12/UI/Effects/
├── SkinnedEmissiveCommon.hlsli    # HLSL 共享定义
├── SkinnedEmissiveVS.hlsl         # 顶点着色器
├── SkinnedEmissivePS.hlsl         # 像素着色器
├── SkinnedEmissiveEffect.h        # C++ 接口
├── SkinnedEmissiveEffect.cpp      # C++ 实现
└── README.md                      # 本文件
```

## 集成步骤

### 1. 在 UIDXFoundation.h 中声明

```cpp
#include "Effects/SkinnedEmissiveEffect.h"

class UIDXFoundation : public SingletonPattern<UIDXFoundation> {
public:
    // 获取自定义 Effect
    AmazeUI::SkinnedEmissiveEffect* GetSkinnedEmissiveEffect() const { 
        return p_skinnedEmissiveEffect.get(); 
    }

private:
    std::unique_ptr<AmazeUI::SkinnedEmissiveEffect> p_skinnedEmissiveEffect;
};
```

### 2. 在 UIDXFoundation.cpp 中创建

```cpp
void UIDXFoundation::CreateDeviceDependentResourcesXTK() {
    // ... 现有代码 ...
    
    // 创建 SkinnedEmissiveEffect
    // 注意：需要在 descriptor heap 中分配一个 slot 给骨骼 SRV
    const size_t boneSRVIndex = Descriptors::ModelTexturesStart + 100;  // 使用一个空闲的索引
    
    p_skinnedEmissiveEffect = std::make_unique<AmazeUI::SkinnedEmissiveEffect>(
        device,
        p_resourceDescriptors->Heap(),
        boneSRVIndex,
        _backBufferFormat,
        _depthBufferFormat
    );
    
    // 配置默认光照
    p_skinnedEmissiveEffect->EnableDefaultLighting();
}
```

### 3. 在 StaticMesh.cpp 中使用

```cpp
void StaticMesh::Render(UICameraBase3D* pCamera, const XMMATRIX& worldMatrix) {
    auto* pDX12 = UIDXFoundation::GetSingletonInstance();
    auto* effect = pDX12->GetSkinnedEmissiveEffect();
    
    if (!effect) return;
    
    auto commandList = pDX12->GetDeviceResources()->GetCommandList();
    
    // 设置变换矩阵
    effect->SetWorld(worldMatrix);
    effect->SetView(pCamera->GetViewMatrix());
    effect->SetProjection(pCamera->GetProjectionMatrix());
    
    // 设置骨骼变换（支持 256 根骨骼！）
    if (!_bones.empty()) {
        std::vector<XMMATRIX> boneTransforms(_bones.size());
        for (size_t i = 0; i < _bones.size(); i++) {
            boneTransforms[i] = XMLoadFloat4x4(&_boneMatrices[i]);
        }
        effect->SetBoneTransforms(boneTransforms.data(), boneTransforms.size());
    }
    
    // 设置缓冲区
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    commandList->IASetIndexBuffer(&_indexBufferView);
    
    // 渲染每个子网格
    for (const auto& submesh : _subMeshes) {
        const auto& mat = _materials[submesh.materialIndex];
        
        // 设置漫反射纹理
        if (mat.hasDiffuseTexture) {
            effect->SetDiffuseTexture(mat.diffuseSRV);
        }
        
        // 🔥 设置自发光纹理（核心新功能！）
        if (mat.hasEmissiveTexture) {
            effect->SetEmissiveTexture(mat.emissiveSRV);
            effect->SetEmissiveStrength(2.0f);  // 调整发光强度
        }
        
        // 应用 Effect 并绘制
        effect->Apply(commandList);
        commandList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.startIndex, 0, 0);
    }
}
```

## API 参考

### 矩阵设置
```cpp
void SetWorld(const XMMATRIX& world);
void SetView(const XMMATRIX& view);
void SetProjection(const XMMATRIX& projection);
```

### 材质属性
```cpp
void SetDiffuseColor(const XMFLOAT4& color);
void SetEmissiveColor(const XMFLOAT4& color);
void SetEmissiveStrength(float strength);    // 发光强度（默认 1.0）
void SetAmbientColor(const XMFLOAT3& color);
```

### 纹理绑定
```cpp
void SetDiffuseTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv);
void SetEmissiveTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv);
```

### 光照设置
```cpp
void SetLightEnabled(int index, bool enabled);          // index: 0-2
void SetLightDirection(int index, const XMFLOAT3& dir);
void SetLightDiffuseColor(int index, const XMFLOAT3& color);
void EnableDefaultLighting();                           // 启用默认三点光照
```

### 骨骼蒙皮（核心功能）
```cpp
// 支持最多 256 根骨骼！
void SetBoneTransforms(const XMMATRIX* transforms, size_t count);
```

### 渲染
```cpp
void Apply(ID3D12GraphicsCommandList* commandList);
```

## 使用示例

### 基础渲染
```cpp
auto* effect = UIDXFoundation::GetSingletonInstance()->GetSkinnedEmissiveEffect();

effect->SetWorld(worldMatrix);
effect->SetView(camera->GetViewMatrix());
effect->SetProjection(camera->GetProjectionMatrix());

effect->SetDiffuseTexture(diffuseSRV);
effect->Apply(commandList);
commandList->DrawIndexedInstanced(...);
```

### 发光效果
```cpp
// 设置发光纹理和颜色
effect->SetEmissiveTexture(emissiveSRV);
effect->SetEmissiveColor(XMFLOAT4(1.0f, 1.0f, 0.5f, 1.0f));  // 黄色发光
effect->SetEmissiveStrength(2.5f);  // 提高发光强度
```

### 骨骼动画
```cpp
// 更新骨骼变换（支持 256 根骨骼）
std::vector<XMMATRIX> bones(88);  // 凤凰模型有 88 根骨骼
UpdateAnimation(bones);  // 更新动画
effect->SetBoneTransforms(bones.data(), bones.size());
```

## 技术细节

### 骨骼存储方式对比

**旧方案（ConstantBuffer）**：
```hlsl
cbuffer Bones : register(b1) {
    float4x4 BoneTransforms[72];  // 限制：4.5 KB
};
```

**新方案（StructuredBuffer）**：
```hlsl
StructuredBuffer<float4x4> BoneTransforms : register(t0);  // 支持：256+ 根骨骼
```

### 着色器计算流程

**顶点着色器（VS）**：
1. 从 StructuredBuffer 读取骨骼矩阵
2. 混合最多 4 根骨骼的影响（蒙皮）
3. 变换顶点到世界空间
4. 投影到屏幕空间

**像素着色器（PS）**：
1. 采样 Diffuse 纹理
2. 采样 Emissive 纹理
3. 计算光照（Lambert 漫反射）
4. 合成：`最终颜色 = (Diffuse × 光照) + Emissive`

## 性能优化

### 批量更新
```cpp
// 一次性更新所有属性
effect->SetMatrices(world, view, proj);
effect->SetBoneTransforms(bones, count);
effect->Apply(commandList);  // 一次性上传到 GPU
```

### 减少状态切换
```cpp
// 相同材质的物体可以复用 Effect 状态
for (auto& mesh : meshes) {
    if (mesh.material != currentMaterial) {
        effect->SetDiffuseTexture(mesh.diffuseSRV);
        effect->SetEmissiveTexture(mesh.emissiveSRV);
        currentMaterial = mesh.material;
    }
    effect->Apply(commandList);
    commandList->DrawIndexedInstanced(...);
}
```

## 已知限制

1. **纹理描述符布局**：Diffuse、Emissive、Normal 纹理必须在描述符堆中连续存储
2. **骨骼数量**：虽然支持 256 根，但需要注意 GPU 内存和性能
3. **光源数量**：当前限制为 3 个方向光

## 未来扩展

- [ ] Normal Mapping（法线贴图）支持
- [ ] PBR 材质系统（Metallic-Roughness）
- [ ] 阴影贴图支持
- [ ] 多 Pass 渲染（透明物体）
- [ ] 实例化渲染（多个相同模型）

## 故障排除

### 问题：模型不显示
- 检查骨骼变换是否已设置
- 检查纹理描述符是否有效
- 检查顶点缓冲区格式是否匹配

### 问题：发光效果不明显
- 增加 `EmissiveStrength`（建议 1.5 - 3.0）
- 检查 Emissive 纹理是否正确加载
- 检查 `EmissiveColor` 是否为白色

### 问题：骨骼动画异常
- 确保骨骼矩阵已正确初始化
- 检查骨骼索引是否超出范围（<256）
- 验证骨骼权重之和是否为 1.0

## 作者

AmazeUI Framework - DirectX 12 Effects

## 版本历史

- **v1.0.0** (2025-10-16): 初始版本
  - 支持 256 根骨骼
  - Emissive 纹理和颜色支持
  - 三点光照系统
