//--------------------------------------------------------------------------------------
// SkinnedEmissivePS.hlsl
// Pixel Shader for Skeletal Animation with Emissive Support
//--------------------------------------------------------------------------------------

#include "SkinnedEmissiveCommon.hlsli"

float4 main(PSInput input) : SV_Target
{
    //==========================================================================
    // Sample textures
    //==========================================================================
    float4 diffuseSample  = DiffuseTexture.Sample(LinearSampler, input.TexCoord);
    float4 emissiveSample = EmissiveTexture.Sample(LinearSampler, input.TexCoord);
    
    //==========================================================================
    // Calculate lighting
    //==========================================================================
    float3 normal = normalize(input.Normal);
    
    // Start with ambient light
    float3 totalLighting = AmbientColor;
    
    // Add directional lights (up to 3 lights)
    for (int i = 0; i < LightCount && i < 3; i++)
    {
        Light light = Lights[i];
        
        // Lambert diffuse lighting: NdotL
        float NdotL = max(dot(normal, -light.Direction), 0.0);
        totalLighting += light.DiffuseColor * NdotL;
    }
    
    //==========================================================================
    // Compose final color
    //==========================================================================
    
    // Diffuse contribution = texture color × material color × lighting
    float3 diffuseContribution = diffuseSample.rgb * DiffuseColor.rgb * totalLighting;
    
    // Emissive contribution = emissive texture × emissive color × strength
    // Note: Emissive is NOT affected by lighting (self-illumination)
    float3 emissiveContribution = emissiveSample.rgb * EmissiveColor.rgb * EmissiveStrength;
    
    // Final color = diffuse + emissive (additive blend)
    float3 finalColor = diffuseContribution + emissiveContribution;
    
    // Optional: Simple tone mapping to prevent over-bright colors
    // Uncomment for HDR tone mapping:
    // finalColor = finalColor / (finalColor + 1.0);  // Reinhard tone mapping
    
    //==========================================================================
    // Alpha channel
    //==========================================================================
    float alpha = diffuseSample.a * DiffuseColor.a;
    
    return float4(finalColor, alpha);
}
