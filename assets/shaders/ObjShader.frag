#version 460

#define NUM_CASCADES 1
#define NUM_PREBAKED 4

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
uniform sampler2D u_ShadowMaps[NUM_CASCADES];
uniform mat4 u_LightSpaceMatrices[NUM_CASCADES];
uniform sampler2D u_PreBakedShadowMaps[NUM_PREBAKED];
uniform mat4 u_PreBakedLightSpaceMatrices[NUM_PREBAKED];

// IBL
uniform samplerCube u_SkyBox;
uniform sampler2D u_BRDFLUT;

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
    for (int i = 0; i < 4; ++i) {
        vec2 offset = rotation * poissonDisk[i] * texelSize;
        float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
        if (depth - bias > sampleDepth) earlyShadowCount++;
    }

    if (earlyShadowCount == 0) return 0.0;
    else if (earlyShadowCount == 4) return 1.0;

    int lastFourUnshadowed = 0;
    for (int i = 0; i < NUM_POISSON_SAMPLES; ++i) {
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
    int bestPrebaked = -1;

    vec3 cascadeProjCoords;
    vec3 prebakedProjCoords;

    float cascadeDepth = 0.0;
    float prebakedDepth = 0.0;

    float bias = max(0.003 * (1.0 - dot(normal, lightDir)), 0.0005);

    for (int i = 0; i < NUM_PREBAKED; ++i) {
        vec4 lightSpacePos = u_PreBakedLightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w * 0.5 + 0.5;
        if (all(greaterThanEqual(projCoords, vec3(0.0))) && all(lessThanEqual(projCoords, vec3(1.0)))) {
            bestPrebaked = i;
            prebakedProjCoords = projCoords;
            prebakedDepth = projCoords.z;
            break;
        }
    }

    for (int i = 0; i < NUM_CASCADES; ++i) {
        vec4 lightSpacePos = u_LightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w * 0.5 + 0.5;
        if (all(greaterThanEqual(projCoords, vec3(0.0))) && all(lessThanEqual(projCoords, vec3(1.0)))) {
            bestCascade = i;
            cascadeProjCoords = projCoords;
            cascadeDepth = projCoords.z;
            break;
        }
    }

    float shadowCascade = 0.0;
    float shadowPrebaked = 0.0;

    if (bestCascade != -1)
        shadowCascade = ComputeShadowPoissonPCF(u_ShadowMaps[bestCascade], cascadeProjCoords, cascadeDepth, bias);

    if (bestPrebaked != -1)
        shadowPrebaked = ComputeShadowPoissonPCF(u_PreBakedShadowMaps[bestPrebaked], prebakedProjCoords, prebakedDepth, bias);

    float shadow = max(shadowCascade, shadowPrebaked);
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
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
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
    vec4 albedoTex = u_HasAlbedoMap ? texture(u_AlbedoMap, v_TexCoord) : u_AlbedoFallback;
    vec3 albedo = albedoTex.rgb * u_BaseColorFactor.rgb;

    float alpha = albedoTex.a * u_BaseColorFactor.a;

    if (u_AlphaMode == 1) { // MASK
        if (alpha < u_AlphaCutoff) discard;
        alpha = 1.0;
    } else if (u_AlphaMode == 0) { // OPAQUE
        alpha = 1.0;
    }

    // Metallic/Roughness (G=roughness, B=metallic)
    float roughnessTex;
    float metallicTex;
    if (u_HasMetallicRoughnessMap) {
        vec2 mr = texture(u_MetallicRoughnessMap, v_TexCoord).gb;
        roughnessTex = mr.x;
        metallicTex  = mr.y;
    } else {
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
    if (u_HasNormalMap) {
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

    // IBL (single skybox)
    float NdotV = max(dot(N, V), 0.0);

    // Diffuse IBL: sample skybox at a high LOD along the normal (approx irradiance)
    const float DIFFUSE_LOD = float(MAX_IBL_LOD);
    vec3 diffuseEnv = textureLod(u_SkyBox, N, DIFFUSE_LOD).rgb;
    vec3 diffuseIBL = diffuseEnv * (kD * albedo);

    // Specular IBL: prefiltered skybox via roughness-based LOD
    vec3 R = reflect(-V, N);
    float lod = roughness * float(MAX_IBL_LOD);
    vec3 envColor = textureLod(u_SkyBox, R, lod).rgb;

    // Roughness-aware Fresnel for IBL
    vec3 Fibl = FresnelSchlickRoughness(NdotV, F0, roughness);

    // Specular IBL
    vec2 brdf = textureLod(u_BRDFLUT, vec2(roughness, 1.0 - NdotV), 0.0).rg;
    vec3 specularIBL = envColor * (Fibl * brdf.x + brdf.y);

    vec3 ambient;
    if (isGlass) {
        // Thin-surface transmission via refracted IBL
        float eta = max(u_IOR, 1.0001); // avoid divide-by-zero
        vec3 Tdir = refract(-V, N, 1.0 / eta);
        vec3 envT = textureLod(u_SkyBox, Tdir, lod).rgb;

        // Energy split: what isn't reflected can transmit, tint by baseColor
        vec3 transIBL = envT * (1.0 - Fibl) * transmission * albedo;

        // No AO on specular/transmission
        ambient = specularIBL + transIBL;
    } else {
        // AO only affects diffuse, not specular
        ambient = ao * diffuseIBL + specularIBL;
    }

    vec3 color = ambient + direct + emissive;

    // After computing Fibl (roughness-aware Fresnel) and 'transmission'
    float outAlpha = alpha;

    if (u_AlphaMode == 2) {
        float transVis = transmission * (1.0 - Fibl.r);      // how much isn't reflected
        float roughBoost = mix(0.0, 0.3, clamp(roughness, 0.0, 1.0));
        float coverage = clamp(0.02 + 0.65 * (transVis + roughBoost), 0.05, 0.7);
        outAlpha = max(alpha, coverage);
    }
    FragColor = vec4(color, outAlpha);
}