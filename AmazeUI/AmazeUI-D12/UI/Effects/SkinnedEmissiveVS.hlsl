//--------------------------------------------------------------------------------------
// SkinnedEmissiveVS.hlsl
// Vertex Shader for Skeletal Animation with Emissive Support
//--------------------------------------------------------------------------------------

#include "SkinnedEmissiveCommon.hlsli"

PSInput main(VSInput input)
{
    PSInput output;
    
    //==========================================================================
    // Skinning: Blend vertex position and normal by bone weights
    //==========================================================================
    float4 skinnedPosition = float4(0, 0, 0, 0);
    float3 skinnedNormal   = float3(0, 0, 0);
    
    // Process up to 4 bone influences per vertex
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        uint boneIndex = input.BoneIndices[i];
        float weight = input.BoneWeights[i];
        
        // Only process if weight > 0 and bone index is valid
        if (weight > 0.0f && boneIndex < MAX_BONES)
        {
            // Fetch bone transform from StructuredBuffer
            float4x4 boneTransform = BoneTransforms[boneIndex];
            
            // Transform position
            skinnedPosition += weight * mul(float4(input.Position, 1.0), boneTransform);
            
            // Transform normal (only rotation, no translation)
            skinnedNormal += weight * mul(input.Normal, (float3x3)boneTransform);
        }
    }
    
    // Fallback for vertices with zero total weight
    if (length(skinnedPosition.xyz) < 0.001)
    {
        skinnedPosition = float4(input.Position, 1.0);
        skinnedNormal = input.Normal;
    }
    
    //==========================================================================
    // Transform to world space
    //==========================================================================
    float4 worldPosition = mul(skinnedPosition, World);
    output.WorldPos = worldPosition.xyz;
    
    //==========================================================================
    // Transform to projection space
    //==========================================================================
    float4 viewPosition = mul(worldPosition, View);
    output.Position = mul(viewPosition, Projection);
    
    //==========================================================================
    // Transform normal to world space
    //==========================================================================
    output.Normal = normalize(mul(skinnedNormal, (float3x3)World));
    
    //==========================================================================
    // Pass through texture coordinates
    //==========================================================================
    output.TexCoord = input.TexCoord;
    
    return output;
}
