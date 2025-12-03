#include "SkinnedEmissiveEffect.h"
#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace AmazeUI {

//======================================================================================
// Compiled Shader Code (Inline HLSL - Based on DirectXTK12)
//======================================================================================

// Shader code based on DirectXTK12's SkinnedEffect with emissive texture support added
static const char g_SkinnedEmissiveShader[] = R"(
//--------------------------------------------------------------------------------------
// SkinnedEmissiveEffect Shader (DirectXTK12 Compatible with Emissive)
//--------------------------------------------------------------------------------------

Texture2D<float4> Texture  : register(t0);  // Diffuse texture (DirectXTK compatible)
Texture2D<float4> Texture2 : register(t1);  // Emissive texture (NEW)
SamplerState      Sampler  : register(s0);

cbuffer Parameters : register(b0)
{
    float4   DiffuseColor             : packoffset(c0);
    float3   EmissiveColor            : packoffset(c1);
    float3   SpecularColor            : packoffset(c2);
    float    SpecularPower            : packoffset(c2.w);
    
    float3   LightDirection[3]        : packoffset(c3);
    float3   LightDiffuseColor[3]     : packoffset(c6);
    float3   LightSpecularColor[3]    : packoffset(c9);
    
    float3   EyePosition              : packoffset(c12);
    
    float3   FogColor                 : packoffset(c13);
    float4   FogVector                : packoffset(c14);
    
    float4x4 World                    : packoffset(c15);
    float3x3 WorldInverseTranspose    : packoffset(c19);
    float4x4 WorldViewProj            : packoffset(c22);
    
    float4x3 Bones[72]                : packoffset(c26);  // DirectXTK: 72 bones max
};

struct VSInputNmTxWeights
{
    float3 Position : SV_Position;  // CHANGED: float3 to match actual vertex data
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    uint4  Indices  : BLENDINDICES0;
    float4 Weights  : BLENDWEIGHT0;
};

struct VSOutputPixelLightingTx
{
    float4 PositionPS : SV_Position;
    float2 TexCoord   : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS   : TEXCOORD2;
    float4 Diffuse    : COLOR0;
};

struct PSInputPixelLightingTx
{
    float2 TexCoord   : TEXCOORD0;
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS   : TEXCOORD2;
    float4 Diffuse    : COLOR0;
};

// Skinning function (from DirectXTK Skinning.fxh)
float3 Skin(inout VSInputNmTxWeights vin, float3 normal, uniform int boneCount)
{
    float4x3 skinning = 0;
    
    [unroll]
    for (int i = 0; i < boneCount; i++)
    {
        skinning += Bones[vin.Indices[i]] * vin.Weights[i];
    }
    
    vin.Position.xyz = mul(vin.Position, skinning);
    return mul(normal, (float3x3)skinning);
}

//--------------------------------------------------------------------------------------
// Vertex Shader: Pixel Lighting (4 bones) - DirectXTK Compatible
//--------------------------------------------------------------------------------------
VSOutputPixelLightingTx VSSkinnedPixelLighting(VSInputNmTxWeights vin)
{
    VSOutputPixelLightingTx vout;
    
    float3 normal = Skin(vin, vin.Normal, 4);
    
    // Transform to world space
    float4 pos_ws = mul(vin.Position, World);
    float3 normal_ws = normalize(mul(normal, WorldInverseTranspose));
    
    vout.PositionPS = mul(vin.Position, WorldViewProj);
    vout.PositionWS = float4(pos_ws.xyz, vout.PositionPS.w);
    vout.NormalWS = normal_ws;
    vout.Diffuse = float4(1, 1, 1, DiffuseColor.a);
    vout.TexCoord = vin.TexCoord;
    
    return vout;
}

//--------------------------------------------------------------------------------------
// Pixel Shader: Pixel Lighting + Emissive (DirectXTK style)
//--------------------------------------------------------------------------------------
float4 PSSkinnedPixelLighting(PSInputPixelLightingTx pin) : SV_Target0
{
    float4 color = Texture.Sample(Sampler, pin.TexCoord) * pin.Diffuse;
    
    float3 eyeVector = normalize(EyePosition - pin.PositionWS.xyz);
    float3 worldNormal = normalize(pin.NormalWS);
    
    // Lighting calculation (DirectXTK style)
    float3 diffuse = 0;
    float3 specular = 0;
    
    for (int i = 0; i < 3; i++)
    {
        float3 lightDir = -LightDirection[i];
        float ndotl = max(dot(worldNormal, lightDir), 0);
        diffuse += LightDiffuseColor[i] * ndotl;
        
        float3 halfVector = normalize(lightDir + eyeVector);
        float ndoth = max(dot(worldNormal, halfVector), 0);
        specular += LightSpecularColor[i] * pow(abs(ndoth), SpecularPower);
    }
    
    color.rgb *= diffuse;
    color.rgb += SpecularColor * specular;
    
    // Add emissive (NEW - only difference from DirectXTK)
    float4 emissiveSample = Texture2.Sample(Sampler, pin.TexCoord);
    color.rgb += emissiveSample.rgb * EmissiveColor;
    
    return color;
}
)";


//======================================================================================
// Private Implementation Structure
//======================================================================================
struct SkinnedEmissiveEffect::Impl {
    // GPU Resources
    ComPtr<ID3D12RootSignature>      rootSignature;
    ComPtr<ID3D12PipelineState>      pipelineState;
    
    // Constant buffers (using upload heap for CPU updates)
    ComPtr<ID3D12Resource>           perFrameConstantBuffer;
    ComPtr<ID3D12Resource>           perObjectConstantBuffer;
    void*                            perFrameCBMappedData = nullptr;
    void*                            perObjectCBMappedData = nullptr;
    
    // Bone transforms StructuredBuffer (supports 256 bones)
    ComPtr<ID3D12Resource>           boneTransformBuffer;         // GPU buffer
    ComPtr<ID3D12Resource>           boneTransformUploadBuffer;   // CPU staging buffer
    D3D12_GPU_DESCRIPTOR_HANDLE      boneTransformSRV = {};
    std::vector<XMFLOAT4X4>          boneMatrices;
    
    // Texture descriptors (must be consecutive in descriptor heap!)
    D3D12_GPU_DESCRIPTOR_HANDLE      textureTableStart = {};  // Points to start of 3 consecutive SRVs
    D3D12_GPU_DESCRIPTOR_HANDLE      diffuseSRV = {};          // Legacy: individual textures
    D3D12_GPU_DESCRIPTOR_HANDLE      emissiveSRV = {};
    D3D12_GPU_DESCRIPTOR_HANDLE      normalSRV = {};
    
    // Compiled shaders
    ComPtr<ID3DBlob>                 vertexShaderBlob;
    ComPtr<ID3DBlob>                 pixelShaderBlob;
    
    // Constant buffer data structures (must match HLSL layout)
    struct PerFrameConstants {
        XMFLOAT4X4 View;
        XMFLOAT4X4 Projection;
        XMFLOAT3   CameraPosition;
        float      Time;
        
        PerFrameConstants() {
            XMStoreFloat4x4(&View, XMMatrixIdentity());
            XMStoreFloat4x4(&Projection, XMMatrixIdentity());
            CameraPosition = XMFLOAT3(0, 0, 0);
            Time = 0.0f;
        }
    } perFrameConstants;
    
    struct Light {
        XMFLOAT3 Direction;
        float    Padding1;
        XMFLOAT3 DiffuseColor;
        float    Padding2;
        
        Light() : Direction(0, -1, 0), Padding1(0), DiffuseColor(1, 1, 1), Padding2(0) {}
    };
    
    struct PerObjectConstants {
        XMFLOAT4X4 World;
        XMFLOAT4   DiffuseColor;
        XMFLOAT4   EmissiveColor;
        XMFLOAT3   AmbientColor;
        float      EmissiveStrength;
        Light      Lights[3];
        int        LightCount;
        int        Padding[3];
        
        PerObjectConstants() {
            XMStoreFloat4x4(&World, XMMatrixIdentity());
            DiffuseColor = XMFLOAT4(1, 1, 1, 1);
            EmissiveColor = XMFLOAT4(1, 1, 1, 1);
            AmbientColor = XMFLOAT3(0.2f, 0.2f, 0.2f);
            EmissiveStrength = 1.0f;
            LightCount = 0;
            Padding[0] = Padding[1] = Padding[2] = 0;
        }
    } perObjectConstants;
    
    // Light state tracking
    bool lightEnabled[3] = { false, false, false };
    
    // Dirty flags
    bool bonesDirty = true;
    bool constantsDirty = true;
};

//======================================================================================
// Constructor
//======================================================================================
SkinnedEmissiveEffect::SkinnedEmissiveEffect(
    ID3D12Device* device,
    ID3D12DescriptorHeap* srvHeap,
    size_t srvDescriptorIndex,
    DXGI_FORMAT renderTargetFormat,
    DXGI_FORMAT depthStencilFormat
) : pImpl(std::make_unique<Impl>()) {
    
    // Initialize bone matrices to identity
    pImpl->boneMatrices.resize(MaxBones);
    XMMATRIX identity = XMMatrixIdentity();
    for (auto& mat : pImpl->boneMatrices) {
        XMStoreFloat4x4(&mat, identity);
    }
    
    // Create resources
    CompileShaders();
    CreateRootSignature(device);
    CreatePipelineState(device, renderTargetFormat, depthStencilFormat);
    CreateConstantBuffers(device);
    CreateBoneStructuredBuffer(device, srvHeap, srvDescriptorIndex);
}

//======================================================================================
// Destructor
//======================================================================================
SkinnedEmissiveEffect::~SkinnedEmissiveEffect() {
    // Unmap constant buffers if mapped
    if (pImpl->perFrameCBMappedData) {
        pImpl->perFrameConstantBuffer->Unmap(0, nullptr);
    }
    if (pImpl->perObjectCBMappedData) {
        pImpl->perObjectConstantBuffer->Unmap(0, nullptr);
    }
}

//======================================================================================
// Compile Shaders
//======================================================================================
void SkinnedEmissiveEffect::CompileShaders() {
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    
    ComPtr<ID3DBlob> errorBlob;
    
    // Compile vertex shader from memory
    HRESULT hr = D3DCompile(
        g_SkinnedEmissiveShader,
        strlen(g_SkinnedEmissiveShader),
        "SkinnedEmissiveVS.hlsl",
        nullptr,
        nullptr,
        "VSSkinnedPixelLighting",  // Entry point
        "vs_5_1",
        compileFlags,
        0,
        &pImpl->vertexShaderBlob,
        &errorBlob
    );
    
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA("Vertex Shader Compilation Error:\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::exception("Failed to compile vertex shader");
    }
    
    // Compile pixel shader from memory
    hr = D3DCompile(
        g_SkinnedEmissiveShader,
        strlen(g_SkinnedEmissiveShader),
        "SkinnedEmissivePS.hlsl",
        nullptr,
        nullptr,
        "PSSkinnedPixelLighting",  // Entry point
        "ps_5_1",
        compileFlags,
        0,
        &pImpl->pixelShaderBlob,
        &errorBlob
    );
    
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA("Pixel Shader Compilation Error:\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::exception("Failed to compile pixel shader");
    }
}

//======================================================================================
// Create Root Signature
//======================================================================================
void SkinnedEmissiveEffect::CreateRootSignature(ID3D12Device* device) {
    // Define descriptor ranges
    CD3DX12_DESCRIPTOR_RANGE1 srvRanges[2];
    srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);  // t0: BoneTransforms
    srvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1);  // t1-t3: Textures
    
    // Define root parameters
    CD3DX12_ROOT_PARAMETER1 rootParameters[4];
    rootParameters[0].InitAsConstantBufferView(0);              // b0: PerFrame
    rootParameters[1].InitAsConstantBufferView(1);              // b1: PerObject
    rootParameters[2].InitAsDescriptorTable(1, &srvRanges[0]);  // Bones
    rootParameters[3].InitAsDescriptorTable(1, &srvRanges[1]);  // Textures
    
    // Define static sampler
    CD3DX12_STATIC_SAMPLER_DESC staticSampler(
        0,  // register s0
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );
    
    // Create root signature description
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(
        _countof(rootParameters), rootParameters,
        1, &staticSampler,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );
    
    // Serialize and create root signature
    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_1,
        &signature,
        &error
    );
    
    if (FAILED(hr)) {
        if (error) {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        throw std::exception("Failed to serialize root signature");
    }
    
    hr = device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&pImpl->rootSignature)
    );
    
    if (FAILED(hr)) {
        throw std::exception("Failed to create root signature");
    }
}

//======================================================================================
// Create Pipeline State
//======================================================================================
void SkinnedEmissiveEffect::CreatePipelineState(
    ID3D12Device* device,
    DXGI_FORMAT renderTargetFormat,
    DXGI_FORMAT depthStencilFormat
) {
    // Define input layout (must match VSInputNmTxWeights in HLSL - DirectXTK format)
    const D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "SV_Position",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",         0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",       0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES",   0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    // Create pipeline state description
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pImpl->rootSignature.Get();
    psoDesc.VS = { pImpl->vertexShaderBlob->GetBufferPointer(), pImpl->vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pImpl->pixelShaderBlob->GetBufferPointer(), pImpl->pixelShaderBlob->GetBufferSize() };
    
    // Blend state (opaque)
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    
    // Rasterizer state
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    
    // Depth-stencil state
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    
    // Input layout
    psoDesc.InputLayout = { inputElements, _countof(inputElements) };
    
    // Primitive topology
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    
    // Render target formats
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = renderTargetFormat;
    psoDesc.DSVFormat = depthStencilFormat;
    
    // Sample description
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = UINT_MAX;
    
    // Create pipeline state
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pImpl->pipelineState));
    if (FAILED(hr)) {
        throw std::exception("Failed to create pipeline state");
    }
}

//======================================================================================
// Create Constant Buffers
//======================================================================================
void SkinnedEmissiveEffect::CreateConstantBuffers(ID3D12Device* device) {
    // Calculate aligned sizes
    const UINT perFrameSize = (sizeof(Impl::PerFrameConstants) + 255) & ~255;
    const UINT perObjectSize = (sizeof(Impl::PerObjectConstants) + 255) & ~255;
    
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    
    // Create per-frame constant buffer
    CD3DX12_RESOURCE_DESC perFrameDesc = CD3DX12_RESOURCE_DESC::Buffer(perFrameSize);
    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &perFrameDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pImpl->perFrameConstantBuffer)
    );
    
    // Create per-object constant buffer
    CD3DX12_RESOURCE_DESC perObjectDesc = CD3DX12_RESOURCE_DESC::Buffer(perObjectSize);
    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &perObjectDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pImpl->perObjectConstantBuffer)
    );
    
    // Map constant buffers (keep them mapped for the lifetime of the effect)
    CD3DX12_RANGE readRange(0, 0);  // We don't read from CPU
    pImpl->perFrameConstantBuffer->Map(0, &readRange, &pImpl->perFrameCBMappedData);
    pImpl->perObjectConstantBuffer->Map(0, &readRange, &pImpl->perObjectCBMappedData);
}

//======================================================================================
// Create Bone Structured Buffer
//======================================================================================
void SkinnedEmissiveEffect::CreateBoneStructuredBuffer(
    ID3D12Device* device,
    ID3D12DescriptorHeap* srvHeap,
    size_t descriptorIndex
) {
    const UINT bufferSize = sizeof(XMFLOAT4X4) * MaxBones;
    
    // Create default heap buffer (GPU-only)
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    
    device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&pImpl->boneTransformBuffer)
    );
    
    // Create upload heap buffer (CPU-writable)
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pImpl->boneTransformUploadBuffer)
    );
    
    // Create SRV for the bone buffer
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MaxBones;
    srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    
    // Get CPU and GPU descriptor handles
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(
        srvHeap->GetCPUDescriptorHandleForHeapStart(),
        static_cast<INT>(descriptorIndex),
        descriptorSize
    );
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(
        srvHeap->GetGPUDescriptorHandleForHeapStart(),
        static_cast<INT>(descriptorIndex),
        descriptorSize
    );
    
    device->CreateShaderResourceView(pImpl->boneTransformBuffer.Get(), &srvDesc, cpuHandle);
    pImpl->boneTransformSRV = gpuHandle;
}

//======================================================================================
// Matrix Setters
//======================================================================================
void SkinnedEmissiveEffect::SetWorld(const XMMATRIX& world) {
    XMStoreFloat4x4(&pImpl->perObjectConstants.World, XMMatrixTranspose(world));
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetView(const XMMATRIX& view) {
    XMStoreFloat4x4(&pImpl->perFrameConstants.View, XMMatrixTranspose(view));
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetProjection(const XMMATRIX& projection) {
    XMStoreFloat4x4(&pImpl->perFrameConstants.Projection, XMMatrixTranspose(projection));
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetMatrices(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection) {
    SetWorld(world);
    SetView(view);
    SetProjection(projection);
}

//======================================================================================
// Material Setters
//======================================================================================
void SkinnedEmissiveEffect::SetDiffuseColor(const XMFLOAT4& color) {
    pImpl->perObjectConstants.DiffuseColor = color;
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetDiffuseColor(FXMVECTOR color) {
    XMStoreFloat4(&pImpl->perObjectConstants.DiffuseColor, color);
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetEmissiveColor(const XMFLOAT4& color) {
    pImpl->perObjectConstants.EmissiveColor = color;
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetEmissiveColor(FXMVECTOR color) {
    XMStoreFloat4(&pImpl->perObjectConstants.EmissiveColor, color);
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetEmissiveStrength(float strength) {
    pImpl->perObjectConstants.EmissiveStrength = strength;
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetAmbientColor(const XMFLOAT3& color) {
    pImpl->perObjectConstants.AmbientColor = color;
    pImpl->constantsDirty = true;
}

void SkinnedEmissiveEffect::SetAmbientColor(FXMVECTOR color) {
    XMStoreFloat3(&pImpl->perObjectConstants.AmbientColor, color);
    pImpl->constantsDirty = true;
}

//======================================================================================
// Texture Setters
//======================================================================================
void SkinnedEmissiveEffect::SetTextures(D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) {
    pImpl->textureTableStart = baseDescriptor;
}

void SkinnedEmissiveEffect::SetDiffuseTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
    pImpl->diffuseSRV = srv;
}

void SkinnedEmissiveEffect::SetEmissiveTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
    pImpl->emissiveSRV = srv;
}

void SkinnedEmissiveEffect::SetNormalTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
    pImpl->normalSRV = srv;
}

//======================================================================================
// Lighting Setters
//======================================================================================
void SkinnedEmissiveEffect::SetLightEnabled(int index, bool enabled) {
    if (index >= 0 && index < 3) {
        pImpl->lightEnabled[index] = enabled;
        
        // Update light count
        pImpl->perObjectConstants.LightCount = 0;
        for (int i = 0; i < 3; i++) {
            if (pImpl->lightEnabled[i]) {
                pImpl->perObjectConstants.LightCount++;
            }
        }
        pImpl->constantsDirty = true;
    }
}

void SkinnedEmissiveEffect::SetLightDirection(int index, const XMFLOAT3& direction) {
    if (index >= 0 && index < 3) {
        XMVECTOR dir = XMLoadFloat3(&direction);
        dir = XMVector3Normalize(dir);
        XMStoreFloat3(&pImpl->perObjectConstants.Lights[index].Direction, dir);
        pImpl->constantsDirty = true;
    }
}

void SkinnedEmissiveEffect::SetLightDirection(int index, FXMVECTOR direction) {
    if (index >= 0 && index < 3) {
        XMVECTOR dir = XMVector3Normalize(direction);
        XMStoreFloat3(&pImpl->perObjectConstants.Lights[index].Direction, dir);
        pImpl->constantsDirty = true;
    }
}

void SkinnedEmissiveEffect::SetLightDiffuseColor(int index, const XMFLOAT3& color) {
    if (index >= 0 && index < 3) {
        pImpl->perObjectConstants.Lights[index].DiffuseColor = color;
        pImpl->constantsDirty = true;
    }
}

void SkinnedEmissiveEffect::SetLightDiffuseColor(int index, FXMVECTOR color) {
    if (index >= 0 && index < 3) {
        XMStoreFloat3(&pImpl->perObjectConstants.Lights[index].DiffuseColor, color);
        pImpl->constantsDirty = true;
    }
}

void SkinnedEmissiveEffect::EnableDefaultLighting() {
    // Three-point lighting setup
    static const XMVECTORF32 defaultDirections[3] = {
        {{ -0.5773f, -0.5773f, -0.5773f, 0.f }},  // Key light (front-top)
        {{  0.7071f, -0.3000f, -0.5000f, 0.f }},  // Fill light (side)
        {{  0.0000f,  0.7071f,  0.7071f, 0.f }}   // Rim light (back)
    };
    
    static const XMVECTORF32 defaultColors[3] = {
        {{ 0.9f, 0.9f, 0.9f, 1.0f }},  // Key light: bright
        {{ 0.5f, 0.5f, 0.6f, 1.0f }},  // Fill light: medium
        {{ 0.4f, 0.4f, 0.5f, 1.0f }}   // Rim light: soft
    };
    
    for (int i = 0; i < 3; i++) {
        SetLightEnabled(i, true);
        SetLightDirection(i, defaultDirections[i]);
        SetLightDiffuseColor(i, defaultColors[i]);
    }
    
    SetAmbientColor(XMFLOAT3(0.6f, 0.6f, 0.6f));
}

//======================================================================================
// Bone Transforms (KEY FEATURE)
//======================================================================================
void SkinnedEmissiveEffect::SetBoneTransforms(const XMMATRIX* transforms, size_t count) {
    count = std::min(count, MaxBones);
    
    // Convert and transpose matrices for HLSL
    for (size_t i = 0; i < count; i++) {
        XMStoreFloat4x4(&pImpl->boneMatrices[i], XMMatrixTranspose(transforms[i]));
    }
    
    pImpl->bonesDirty = true;
}

void SkinnedEmissiveEffect::SetBoneTransforms(
    const XMMATRIX* transforms,
    size_t count,
    ID3D12GraphicsCommandList* commandList
) {
    SetBoneTransforms(transforms, count);
    
    // Immediately upload to GPU if command list is provided
    if (pImpl->bonesDirty && commandList) {
        void* mappedData;
        pImpl->boneTransformUploadBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, pImpl->boneMatrices.data(), sizeof(XMFLOAT4X4) * MaxBones);
        pImpl->boneTransformUploadBuffer->Unmap(0, nullptr);
        
        // Copy to GPU buffer
        commandList->CopyResource(pImpl->boneTransformBuffer.Get(), pImpl->boneTransformUploadBuffer.Get());
        
        // Transition to shader resource
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            pImpl->boneTransformBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );
        commandList->ResourceBarrier(1, &barrier);
        
        pImpl->bonesDirty = false;
    }
}

//======================================================================================
// Apply Effect
//======================================================================================
void SkinnedEmissiveEffect::Apply(ID3D12GraphicsCommandList* commandList) {
    // Update constant buffers if dirty
    if (pImpl->constantsDirty) {
        memcpy(pImpl->perFrameCBMappedData, &pImpl->perFrameConstants, sizeof(Impl::PerFrameConstants));
        memcpy(pImpl->perObjectCBMappedData, &pImpl->perObjectConstants, sizeof(Impl::PerObjectConstants));
        pImpl->constantsDirty = false;
    }
    
    // Update bone transforms if dirty
    if (pImpl->bonesDirty) {
        void* mappedData;
        pImpl->boneTransformUploadBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, pImpl->boneMatrices.data(), sizeof(XMFLOAT4X4) * MaxBones);
        pImpl->boneTransformUploadBuffer->Unmap(0, nullptr);
        
        commandList->CopyResource(pImpl->boneTransformBuffer.Get(), pImpl->boneTransformUploadBuffer.Get());
        
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            pImpl->boneTransformBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );
        commandList->ResourceBarrier(1, &barrier);
        
        pImpl->bonesDirty = false;
    }
    
    // Set pipeline state and root signature
    commandList->SetPipelineState(pImpl->pipelineState.Get());
    commandList->SetGraphicsRootSignature(pImpl->rootSignature.Get());
    
    // Set constant buffers
    commandList->SetGraphicsRootConstantBufferView(0, pImpl->perFrameConstantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, pImpl->perObjectConstantBuffer->GetGPUVirtualAddress());
    
    // Set bone transforms
    commandList->SetGraphicsRootDescriptorTable(2, pImpl->boneTransformSRV);
    
    // Set textures (3 consecutive SRVs: Diffuse, Emissive, Normal)
    if (pImpl->textureTableStart.ptr != 0) {
        commandList->SetGraphicsRootDescriptorTable(3, pImpl->textureTableStart);
    }
    else if (pImpl->diffuseSRV.ptr != 0) {
        // Legacy path: use diffuse texture as base (assumes emissive/normal follow)
        commandList->SetGraphicsRootDescriptorTable(3, pImpl->diffuseSRV);
    }
}

//======================================================================================
// Sampler Mode (for future extension)
//======================================================================================
void SkinnedEmissiveEffect::SetSamplerMode(SamplerMode mode) {
    // Currently using static sampler defined in root signature
    // This could be extended to use dynamic samplers if needed
}

} // namespace AmazeUI
