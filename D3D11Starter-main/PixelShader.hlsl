#include "ShaderIncludes.hlsli"

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);
Texture2D ShadowMap : register(t4);
SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

// CONSTANTS ===================

// A constant Fresnel value for non-metals (glass and plastic have values of about 0.04)
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

static const float PI = 3.14159265359f;


cbuffer DataFromCpu : register(b0)
{
    int specMap;
    
    float4 colorTint;
    float3 cameraPosition;
    float3 ambient;
    float2 uvScale;
    float roughness;
    
    Light directionalLight1;
    Light directionalLight2;
    Light directionalLight3;
    Light pointLight1;
    Light pointLight2;
    
    int fogType;
    float3 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
}


// PBR FUNCTIONS ================

// Lambert diffuse BRDF - Same as the basic lighting diffuse calculation!
// - NOTE: this function assumes the vectors are already NORMALIZED!
float DiffusePBR(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}

// Calculates diffuse amount based on energy conservation
//
// diffuse   - Diffuse amount
// F         - Fresnel result from microfacet BRDF
// metalness - surface metalness amount 
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// Normal Distribution Function: GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
	// Pre-calculations
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	// Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
    float VdotH = saturate(dot(v, h));

	// Final value
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
//
// n - Normal
// v - View vector
//
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
//
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
	// End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));

	// Final value
	// Note: Numerator should be NdotV (or NdotL depending on parameters).
	// However, these are also in the BRDF's denominator, so they'll cancel!
	// We're leaving them out here AND in the BRDF function as the
	// dot products can get VERY small and cause rounding errors.
    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
	// Other vectors
    float3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
	// Pass F out of the function for diffuse balance
    F_out = F;

	// Final specular formula
	// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
	// canceled out by our G() term.  As such, they have been removed
	// from BOTH places to prevent floating point rounding errors.
    float3 specularResult = (D * F * G) / 4;

	// One last non-obvious requirement: According to the rendering equation,
	// specular must have the same NdotL applied as diffuse!  We'll apply
	// that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}

// OLD LIGHT CALCULATIONS =========

float3 PointLightDir(Light light,VertexToPixel input)
{
    return float3(normalize(input.worldPosition - light.Position));
}

float Attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}


float4 lightCalc(Light light, VertexToPixel input, float3 surfaceColor, float3 normal, float rough, float metalness)
{
    float3 dirToLight = -light.Direction;
    if (light.Type == LIGHT_TYPE_POINT)
    {
        dirToLight = PointLightDir(light, input);
    }
    
    
    float3 diffuseTerm = DiffusePBR(normal, dirToLight);
    
    // Specular
    float3 V = normalize(cameraPosition - input.worldPosition);
    float3 F;
    float3 specTerm = 0.0f;
    specTerm = MicrofacetBRDF(normal, dirToLight, V, rough, surfaceColor, F);
    
    float3 balancedDiff = DiffuseEnergyConserve(diffuseTerm, F, metalness);
    
    float3 total = (balancedDiff * surfaceColor + specTerm) * light.Intensity * light.Color;
    
    return float4(total, 1);
}



// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    // Persepctive fixing
    input.shadowMapPos /= input.shadowMapPos.w;
    
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y;
    
    float distToLight = input.shadowMapPos.z;
    float shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowUV, distToLight);
    
    
    
    // Normals
    float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;
    unpackedNormal = normalize(unpackedNormal);
    
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    T = normalize(T - N * dot(T, N));
    float3 B = cross(T, N);
    float3x3 TBN = float3x3(T, B, N);
    
    input.normal = mul(unpackedNormal, TBN);
    
    // Albedo(SurfaceColor)
    float3 surfaceColor = pow(Albedo.Sample(BasicSampler, input.uv * uvScale).rgb, 2.2f);
    
    // Rough
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    
    // Metallic
    float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;
    
    // Specular
    // !Assume albedo texture is actually holding specular color where metalness == 1
    // Note the use of lerp here - metal is generally 0 or 1, but might be in between
    // because of linear texture sampling, so we lerp the specular color to match!
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    // Lights
    float3 color = 0;
    color += lightCalc(directionalLight1, input, surfaceColor, input.normal,roughness,metalness);
    color += lightCalc(directionalLight2, input, surfaceColor, input.normal,roughness,metalness);
    color += lightCalc(directionalLight3, input, surfaceColor, input.normal,roughness,metalness);
    
    color += lightCalc(pointLight1, input, surfaceColor,input.normal,roughness,metalness) * Attenuate(pointLight1, input.worldPosition);
    color += lightCalc(pointLight2, input, surfaceColor,input.normal,roughness,metalness) * Attenuate(pointLight2, input.worldPosition);
    
    color += (surfaceColor * ambient * shadowAmount);
    
    // fog
    float fog = 0.0f;
    float surfaceDistance = distance(cameraPosition, input.worldPosition);
    
    
    switch (fogType)
    {
        case 0:
            fog = 0;
            break;
        case 1:
            fog = smoothstep(fogStart, fogEnd, surfaceDistance);
            break;
        case 2:
            fog = 1.0f - exp(-surfaceDistance * fogDensity);
            break;
    }
    
    color = lerp(color, fogColor, fog);
    
    return float4(pow(color, 1.0f / 2.2f), 1);
}

