//--------------------------------------------------------------------------------------
// SkinnedEmissiveCommon.hlsli
// Common definitions for Skinned Emissive Effect
//--------------------------------------------------------------------------------------

#ifndef SKINNED_EMISSIVE_COMMON_HLSLI
#define SKINNED_EMISSIVE_COMMON_HLSLI

// Maximum number of bones (breaking DirectXTK's 72 bone limit)
#define MAX_BONES 256

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// Vertex Input
struct VSInput
{
    float3 Position     : SV_Position;
    float3 Normal       : NORMAL;
    float2 TexCoord     : TEXCOORD0;
    uint4  BoneIndices  : BLENDINDICES;
    float4 BoneWeights  : BLENDWEIGHT;
};

// Pixel Shader Input
struct PSInput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
};

// Light structure
struct Light
{
    float3 Direction;
    float  Padding1;
    float3 DiffuseColor;
    float  Padding2;
};

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

// Per-frame constants (updated every frame)
cbuffer PerFrameConstants : register(b0)
{
    float4x4 View;
    float4x4 Projection;
    float3   CameraPosition;
    float    Time;
};

// Per-object constants (updated per object/material)
cbuffer PerObjectConstants : register(b1)
{
    float4x4 World;
    float4   DiffuseColor;
    float4   EmissiveColor;
    float3   AmbientColor;
    float    EmissiveStrength;
    Light    Lights[3];
    int      LightCount;
    int      Padding[3];
};

//--------------------------------------------------------------------------------------
// Resources
//--------------------------------------------------------------------------------------

// Bone transforms using StructuredBuffer (supports 256 bones)
StructuredBuffer<float4x4> BoneTransforms : register(t0);

// Textures
Texture2D    DiffuseTexture  : register(t1);
Texture2D    EmissiveTexture : register(t2);
Texture2D    NormalTexture   : register(t3);  // Optional: for future normal mapping

// Sampler
SamplerState LinearSampler   : register(s0);

#endif // SKINNED_EMISSIVE_COMMON_HLSLI
