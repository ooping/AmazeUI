#pragma once

#include "../UIUtility.h"

namespace AmazeUI {

//--------------------------------------------------------------------------------------
// SkinnedEmissiveEffect
// Custom effect for skeletal animation with emissive (glow) support
// 
// Features:
// - Supports up to 256 bones (vs DirectXTK's 72 limit)
// - Full emissive texture and color support
// - Three-point lighting system
// - Uses StructuredBuffer for bone transforms (more efficient)
//--------------------------------------------------------------------------------------
class SkinnedEmissiveEffect {
public:
    // Maximum number of bones supported (breaking DirectXTK's 72 bone limit)
    static constexpr size_t MaxBones = 256;
    
    //==================================================================================
    // Construction / Destruction
    //==================================================================================
    SkinnedEmissiveEffect(
        ID3D12Device* device,
        ID3D12DescriptorHeap* srvHeap,
        size_t srvDescriptorIndex,  // Index in descriptor heap for bone buffer SRV
        DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D32_FLOAT
    );
    
    ~SkinnedEmissiveEffect();
    
    // Non-copyable
    SkinnedEmissiveEffect(const SkinnedEmissiveEffect&) = delete;
    SkinnedEmissiveEffect& operator=(const SkinnedEmissiveEffect&) = delete;
    
    //==================================================================================
    // Matrix Transforms
    //==================================================================================
    void SetWorld(const DirectX::XMMATRIX& world);
    void SetView(const DirectX::XMMATRIX& view);
    void SetProjection(const DirectX::XMMATRIX& projection);
    void SetMatrices(const DirectX::XMMATRIX& world, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);
    
    //==================================================================================
    // Material Properties
    //==================================================================================
    void SetDiffuseColor(const DirectX::XMFLOAT4& color);
    void SetDiffuseColor(DirectX::FXMVECTOR color);
    
    void SetEmissiveColor(const DirectX::XMFLOAT4& color);
    void SetEmissiveColor(DirectX::FXMVECTOR color);
    void SetEmissiveStrength(float strength);
    
    void SetAmbientColor(const DirectX::XMFLOAT3& color);
    void SetAmbientColor(DirectX::FXMVECTOR color);
    
    //==================================================================================
    // Texture Binding
    // IMPORTANT: Textures must be in consecutive descriptor heap slots:
    //   baseDescriptor + 0 = Diffuse Texture (t1)
    //   baseDescriptor + 1 = Emissive Texture (t2)
    //   baseDescriptor + 2 = Normal Texture (t3)
    //==================================================================================
    void SetTextures(D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);
    
    // Legacy API (deprecated): Individual texture setters
    void SetDiffuseTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv);
    void SetEmissiveTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv);
    void SetNormalTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv);  // Reserved for future use
    
    //==================================================================================
    // Lighting (supports up to 3 directional lights)
    //==================================================================================
    void SetLightEnabled(int index, bool enabled);
    void SetLightDirection(int index, const DirectX::XMFLOAT3& direction);
    void SetLightDirection(int index, DirectX::FXMVECTOR direction);
    void SetLightDiffuseColor(int index, const DirectX::XMFLOAT3& color);
    void SetLightDiffuseColor(int index, DirectX::FXMVECTOR color);
    
    // Enable default three-point lighting
    void EnableDefaultLighting();
    
    //==================================================================================
    // Bone Transforms (KEY FEATURE: Supports 256 bones via StructuredBuffer)
    //==================================================================================
    void SetBoneTransforms(const DirectX::XMMATRIX* transforms, size_t count);
    void SetBoneTransforms(DirectX::XMMATRIX const* transforms, size_t count, ID3D12GraphicsCommandList* commandList);
    
    //==================================================================================
    // Rendering
    //==================================================================================
    void Apply(ID3D12GraphicsCommandList* commandList);
    
    //==================================================================================
    // Sampler States
    //==================================================================================
    enum class SamplerMode {
        PointClamp,
        PointWrap,
        LinearClamp,
        LinearWrap,
        AnisotropicClamp,
        AnisotropicWrap
    };
    void SetSamplerMode(SamplerMode mode);

private:
    // Private implementation (Pimpl idiom)
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Initialization helpers
    void CreateRootSignature(ID3D12Device* device);
    void CreatePipelineState(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, DXGI_FORMAT depthStencilFormat);
    void CompileShaders();
    void CreateConstantBuffers(ID3D12Device* device);
    void CreateBoneStructuredBuffer(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, size_t descriptorIndex);
};

} // namespace AmazeUI
