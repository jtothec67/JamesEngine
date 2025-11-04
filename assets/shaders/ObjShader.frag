#version 460

#define MAX_NUM_CASCADES 5
#define MAX_NUM_PREBAKED 16

#define MAX_IBL_LOD 5

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_ViewPos;

uniform int   u_AlphaMode;    // 0 OPAQUE, 1 MASK, 2 BLEND
uniform float u_AlphaCutoff;  // used when u_AlphaMode == 1

// Texture samplers
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicRoughnessMap;
uniform sampler2D u_OcclusionMap;
uniform sampler2D u_EmissiveMap;

// "Has map" flags (bools)
uniform bool u_HasAlbedoMap;
uniform bool u_HasNormalMap;
uniform bool u_HasMetallicRoughnessMap;
uniform bool u_HasOcclusionMap;
uniform bool u_HasEmissiveMap;

// Factors
uniform vec4  u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;

// Fallback values
uniform vec4  u_AlbedoFallback;      // default vec4(1,1,1,1)
uniform float u_MetallicFallback;    // default 0.0
uniform float u_RoughnessFallback;   // default 1.0
uniform float u_AOFallback;          // default 1.0
uniform vec3  u_EmissiveFallback;    // default vec3(0.0)

// Extra core factors from glTF
uniform float u_NormalScale;         // default 1.0
uniform float u_OcclusionStrength;   // default 1.0
uniform vec3 u_EmissiveFactor;       // default vec3(0.0)

// Thin-glass support (KHR_materials_transmission + ior)
uniform float u_TransmissionFactor;  // default 0.0
uniform bool u_HasTransmissionTex;   // default false
uniform sampler2D u_TransmissionTex; // R channel if present
uniform float u_IOR;                 // default 1.5

// Direct light (directional)
uniform vec3 u_DirLightDirection;
uniform vec3 u_DirLightColor;

// Shadowing
uniform int u_NumCascades;
uniform sampler2D u_ShadowMaps[MAX_NUM_CASCADES];
uniform mat4 u_LightSpaceMatrices[MAX_NUM_CASCADES];

// IBL
uniform samplerCube u_IrradianceCube;
uniform samplerCube u_PrefilterEnv;
uniform float u_PrefilterMaxLOD;
uniform sampler2D u_BRDFLUT;

// Post process (but it isn't because we are doing it while processing)
in vec2 v_ScreenUV;

// SSAO
uniform sampler2D u_SSAO;        // AO texture: 1=open, 0=occluded
uniform float u_AOStrength;  // 0..1 (how much to dim diffuse IBL)
uniform float u_AOSpecScale; // 0..1 (gentler dim on specular IBL)
uniform float u_AOMin;       // 0..1 (floor to avoid pitch-black, e.g. 0.05)
uniform vec2 u_InvColorResolution; // 1/width, 1/height of color buffer

out vec4 FragColor;

const int NUM_POISSON_SAMPLES = 12;

vec2 poissonDisk[NUM_POISSON_SAMPLES] = vec2[](
    vec2(0.0,  1.5),
    vec2(0.0, -1.5),
    vec2(-1.5, 0.0),
    vec2(1.5,  0.0),
    vec2(-0.613,  0.245),
    vec2(0.535,  0.423),
    vec2(-0.245, -0.567),
    vec2(0.126,  0.946),
    vec2(-0.931,  0.081),
    vec2(0.804, -0.374),
    vec2(-0.321,  0.715),
    vec2(0.255, -0.803)
);

float ComputeShadowPoissonPCF(sampler2D shadowMap, vec3 projCoords, float depth, float bias)
{
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;
    float totalSamples = 0.0;

    ivec2 screenCoord = ivec2(gl_FragCoord.xy);
    float angle = fract(sin(dot(vec2(screenCoord), vec2(12.9898, 78.233))) * 43758.5453) * 6.2831;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    int earlyShadowCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        vec2 offset = rotation * poissonDisk[i] * texelSize;
        float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
        if (depth - bias > sampleDepth) earlyShadowCount++;
    }

    if (earlyShadowCount == 0) return 0.0;
    else if (earlyShadowCount == 4) return 1.0;

    int lastFourUnshadowed = 0;
    for (int i = 0; i < NUM_POISSON_SAMPLES; ++i)
    {
        vec2 offset = rotation * poissonDisk[i] * texelSize;
        float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
        float isShadowed = (depth - bias > sampleDepth) ? 1.0 : 0.0;

        shadow += isShadowed;
        totalSamples += 1.0;

        if (isShadowed == 0.0) lastFourUnshadowed++;
        else lastFourUnshadowed = 0;

        if (i >= 4 && lastFourUnshadowed >= 4) break;
    }

    return shadow / totalSamples;
}

float ShadowCalculation(vec3 fragWorldPos, vec3 normal, vec3 lightDir)
{
    int bestCascade = -1;

    vec3 cascadeProjCoords;

    float cascadeDepth = 0.0;

    float bias = max(0.003 * (1.0 - dot(normal, lightDir)), 0.0005);

    for (int i = 0; i < u_NumCascades; ++i)
    {
        vec4 lightSpacePos = u_LightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w * 0.5 + 0.5;
        if (all(greaterThanEqual(projCoords, vec3(0.0))) && all(lessThanEqual(projCoords, vec3(1.0))))
        {
            bestCascade = i;
            cascadeProjCoords = projCoords;
            cascadeDepth = projCoords.z;
            break;
        }
    }

    float shadowCascade = 0.0;

    if (bestCascade != -1)
        shadowCascade = ComputeShadowPoissonPCF(u_ShadowMaps[bestCascade], cascadeProjCoords, cascadeDepth, bias);

    float shadow = shadowCascade;
    return shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.14159 * denom * denom + 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness*roughness;
    float k = (a + 1)*(a + 1) / 8;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    // Roughness-aware Fresnel used for IBL approximation
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Albedo + alpha
    vec4 albedoTex;

    if (u_AlphaMode == 1)
    {
        // MASK: clamp sampled mip level
        if (u_HasAlbedoMap)
        {
            float lod = textureQueryLod(u_AlbedoMap, v_TexCoord).x; // Hardware-chosen LOD
            float clampedLod = min(lod, 3.0);                       // Cap at mip 3 (specifically for fences)
            albedoTex = textureLod(u_AlbedoMap, v_TexCoord, clampedLod);
        }
        else
        {
            albedoTex = u_AlbedoFallback;
        }
    }
    else
    {
        // OPAQUE / BLEND: normal sampling
        albedoTex = u_HasAlbedoMap ? texture(u_AlbedoMap, v_TexCoord) : u_AlbedoFallback;
    }

    vec3  albedo = albedoTex.rgb * u_BaseColorFactor.rgb;
    float alpha  = albedoTex.a   * u_BaseColorFactor.a;

    if (u_AlphaMode == 1)
    { // MASK
        if (alpha < u_AlphaCutoff) discard;
        alpha = 1.0;
    }
    else if (u_AlphaMode == 0)
    { // OPAQUE
        alpha = 1.0;
    }

    // Metallic/Roughness (G=roughness, B=metallic)
    float roughnessTex;
    float metallicTex;
    if (u_HasMetallicRoughnessMap)
    {
        vec2 mr = texture(u_MetallicRoughnessMap, v_TexCoord).gb;
        roughnessTex = mr.x;
        metallicTex  = mr.y;
    }
    else
    {
        roughnessTex = u_RoughnessFallback;
        metallicTex  = u_MetallicFallback;
    }
    float metallic  = clamp(u_MetallicFactor  * metallicTex,  0.0, 1.0);
    float roughness = clamp(u_RoughnessFactor * roughnessTex, 0.04, 1.0);

    // AO (R channel)
    float aoSample = u_HasOcclusionMap ? texture(u_OcclusionMap, v_TexCoord).r : u_AOFallback;
    float ao = mix(1.0, aoSample, u_OcclusionStrength);

    // Emissive
    vec3 emissiveTex = u_HasEmissiveMap ? texture(u_EmissiveMap, v_TexCoord).rgb : u_EmissiveFallback;
    vec3 emissive = emissiveTex * u_EmissiveFactor;

    // Normal mapping
    vec3 Ngeom = normalize(v_Normal);
    vec3 N = Ngeom ;
    if (u_HasNormalMap)
    {
        vec3 dp1 = dFdx(v_FragPos);
        vec3 dp2 = dFdy(v_FragPos);
        vec2 duv1 = dFdx(v_TexCoord);
        vec2 duv2 = dFdy(v_TexCoord);
        vec3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
        vec3 B = normalize(-dp1 * duv2.x + dp2 * duv1.x);
        mat3 TBN = mat3(T, B, Ngeom);
        vec3 nSample = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
        nSample.xy *= u_NormalScale;
        nSample.z = sqrt(max(0.0, 1.0 - dot(nSample.xy, nSample.xy)));
        N = normalize(TBN * normalize(nSample));
    }

    if (!gl_FrontFacing) N = -N;

    vec3 V = normalize(u_ViewPos - v_FragPos);
    vec3 L = normalize(-u_DirLightDirection);
    vec3 H = normalize(V + L);

    // Base reflectance
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance direct light
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float shadow = ShadowCalculation(v_FragPos, N, L);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = numerator / denom;

    // Some exporters put transmission only in the texture and leave factor = 0.
    // Read the texture first, then multiply.
    float tTex = u_HasTransmissionTex ? texture(u_TransmissionTex, v_TexCoord).r : 1.0;
    float transmission = clamp(u_TransmissionFactor * tTex, 0.0, 1.0);

    // Treat as "glass" if either factor or texture wants it.
    bool isGlass = (u_TransmissionFactor > 0.001) || (u_HasTransmissionTex && tTex > 0.001);

    // For glass: no diffuse
    if (isGlass) kD = vec3(0.0);

    vec3 radiance = u_DirLightColor;
    vec3 direct = (kD * albedo / 3.14159 + specular) * radiance * NdotL * (1.0 - shadow);

    // IBL
    float NdotV = max(dot(N, V), 0.0);

    // Diffuse IBL: sample skybox at a high LOD along the normal (approx irradiance)
    vec3 diffuseEnv = texture(u_IrradianceCube, N).rgb;
    vec3 diffuseIBL = diffuseEnv * (kD * albedo);

    // Specular IBL: prefiltered skybox via roughness-based LOD
    vec3 R = reflect(-V, N);
    float lod = roughness * u_PrefilterMaxLOD;
    vec3  envColor = textureLod(u_PrefilterEnv, R, lod).rgb;

    vec3 Fibl = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec2 brdf = texture(u_BRDFLUT, vec2(NdotV, roughness)).rg;

    vec3 specularIBL = envColor * (Fibl * brdf.x + brdf.y);

    // Sample SSAO and build attenuation terms
    vec2 aoUV = gl_FragCoord.xy * u_InvColorResolution;
    float ssao = texture(u_SSAO, aoUV).r;
    float aoDiffuse = mix(1.0, max(ssao, u_AOMin), u_AOStrength);
    float aoSpec = mix(1.0, sqrt(ssao), u_AOSpecScale);

    vec3 ambient;
    if (isGlass)
    {
        // Thin-surface transmission via refracted IBL
        float eta  = max(u_IOR, 1.0001);
        vec3  Tdir = refract(-V, N, 1.0 / eta);
        vec3  envT = textureLod(u_PrefilterEnv, Tdir, lod).rgb;

        vec3 transIBL = envT * (1.0 - Fibl) * transmission * albedo; // energy split
        ambient = specularIBL + transIBL;
    }
    else
    {
        // AO only affects diffuse, not specular
        ambient = (ao * aoDiffuse) * diffuseIBL + (aoSpec) * specularIBL;
    }

    vec3 color = ambient + direct + emissive;

    // After computing Fibl (roughness-aware Fresnel) and 'transmission'
    float outAlpha = alpha;

    if (u_AlphaMode == 2)
    {
        float transVis = transmission * (1.0 - Fibl.r);
        float roughBoost = mix(0.0, 0.3, clamp(roughness, 0.0, 1.0));
        float coverage = clamp(0.02 + 0.65 * (transVis + roughBoost), 0.05, 0.7);
        outAlpha = max(alpha, coverage);
    }
    FragColor = vec4(color, outAlpha);
}