#version 460

#define MAX_NUM_CASCADES 5
#define MAX_NUM_PREBAKED 16

#define MAX_IBL_LOD 5

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_ViewPos;

uniform int u_AlphaMode;    // 0 OPAQUE, 1 MASK, 2 BLEND
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
uniform float u_DirLightIntensity;

// Shadowing
uniform int u_NumCascades;
uniform sampler2D u_ShadowMaps[MAX_NUM_CASCADES];
uniform mat4 u_LightSpaceMatrices[MAX_NUM_CASCADES];
uniform float u_CascadeTexelScale[MAX_NUM_CASCADES];
uniform float u_CascadeWorldTexelSize[MAX_NUM_CASCADES];

uniform float u_ShadowBiasSlope;
uniform float u_ShadowBiasMin;
uniform float u_NormalOffsetScale;

// PCSS
uniform float u_PCSSBase;
uniform float u_PCSSScale;

// IBL
uniform samplerCube u_IrradianceCube;
uniform samplerCube u_PrefilterEnv;
uniform float u_PrefilterMaxLOD;
uniform sampler2D u_BRDFLUT;
uniform float u_EnvIntensity;

// Post process (but it isn't because we are doing it while processing)
in vec2 v_ScreenUV;

// SSAO
uniform bool u_UseSSAO;
uniform sampler2D u_SSAO; // AO texture: 1=open, 0=occluded
uniform float u_AOStrength; // 0..1 (how much to dim diffuse IBL)
uniform float u_AOSpecScale; // 0..1 (gentler dim on specular IBL)
uniform float u_AOMin; // 0..1 (floor to avoid pitch-black, e.g. 0.05)
uniform vec2 u_InvColorResolution; // 1/width, 1/height of color buffer

// Temp debug
uniform mat4 u_DebugVP;

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

float ComputeShadowPoissonPCF(sampler2D shadowMap, vec3 projCoords, float depth, float bias, int cascadeIndex)
{
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    // Scale so that the *world-space* blur size is similar across cascades
    float texelScale = u_CascadeTexelScale[cascadeIndex];
    vec2 filterStep  = texelSize * texelScale;

    float shadow = 0.0;
    float totalSamples = 0.0;

    ivec2 screenCoord = ivec2(gl_FragCoord.xy);
    float angle = fract(sin(dot(vec2(screenCoord), vec2(12.9898, 78.233))) * 43758.5453) * 6.2831;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    int earlyShadowCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        vec2 offset = rotation * poissonDisk[i] * filterStep;
        float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
        if (depth - bias > sampleDepth) earlyShadowCount++;
    }

    if (earlyShadowCount == 0) return 0.0;
    else if (earlyShadowCount == 4) return 1.0;

    int lastFourUnshadowed = 0;
    for (int i = 0; i < NUM_POISSON_SAMPLES; ++i)
    {
        vec2 offset = rotation * poissonDisk[i] * filterStep;
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


float ComputeShadowPCSS(sampler2D shadowMap, vec3 projCoords, float receiverDepth, float bias, int cascadeIndex)
{
    float u_PCSS_SearchRadiusTexels = 6;
    float u_PCSS_LightRadiusUV = 0.002;

//    // Early reject: outside shadow map
//    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
//        projCoords.y < 0.0 || projCoords.y > 1.0)
//        return 0.0;

    // Per-pixel Poisson rotation (same seed as you used)
    ivec2 screenCoord = ivec2(gl_FragCoord.xy);
    float angle = fract(sin(dot(vec2(screenCoord), vec2(12.9898, 78.233))) * 43758.5453) * 6.2831853;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    // Map characteristics
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float baseTexel  = max(texelSize.x, texelSize.y);

    // Cascade scale: 1.0 for reference cascade, <1 or >1 for others
    float texelScale = u_CascadeTexelScale[cascadeIndex];
    float searchRadiusUV = u_PCSS_SearchRadiusTexels * baseTexel * texelScale;

    float blockerDepthSum = 0.0;
    float blockerCount = 0.0;

    // Use a subset of Poisson points for blocker search (12 is common)
    const int BLOCKER_SAMPLES = 12;
    for (int i = 0; i < BLOCKER_SAMPLES; ++i)
    {
        vec2 offsetUV = rotation * poissonDisk[i] * searchRadiusUV;
        float sampleDepth = texture(shadowMap, projCoords.xy + offsetUV).r;

        // "Closer than receiver" means this sample is a potential blocker.
        if (receiverDepth - bias > sampleDepth)
        {
            blockerDepthSum += sampleDepth;
            blockerCount += 1.0;
        }
    }

    // If no blockers found, fully lit
    if (blockerCount == 0.0)
        return 0.0;

    float avgBlockerDepth = blockerDepthSum / blockerCount;

    float receiverMinusBlocker = max(receiverDepth - avgBlockerDepth, 0.0);
    float penumbraUV = u_PCSS_LightRadiusUV * (receiverMinusBlocker / max(avgBlockerDepth, 1e-5));

    // Original penumbra in local texels
    float penumbraTexelsLocal = u_PCSSBase + ((penumbraUV / baseTexel) * u_PCSSScale);

    // Convert to a "reference cascade" texel radius
    float penumbraTexelsRef = penumbraTexelsLocal * texelScale;

    float filterRadiusUV = penumbraTexelsRef * baseTexel;

    float shadow = 0.0;
    float total  = 0.0;

    // Optional tiny early-out probe (very cheap 4-tap test)
    int earlyShadowCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        vec2 offsetUV = rotation * poissonDisk[i] * filterRadiusUV;
        float s = texture(shadowMap, projCoords.xy + offsetUV).r;
        if (receiverDepth - bias > s) earlyShadowCount++;
    }
//    if (earlyShadowCount == 0) return 0.0;
//    if (earlyShadowCount == 4 && penumbraTexels <= 1.0) return 1.0; // fully occluded & tiny penumbra

    int lastFourUnshadowed = 0;
    for (int i = 0; i < NUM_POISSON_SAMPLES; ++i)
    {
        vec2 offsetUV = rotation * poissonDisk[i] * filterRadiusUV;
        float sampleDepth = texture(shadowMap, projCoords.xy + offsetUV).r;
        float isShadowed = (receiverDepth - bias > sampleDepth) ? 1.0 : 0.0;

        shadow += isShadowed;
        total  += 1.0;

        if (isShadowed == 0.0) lastFourUnshadowed++;
        else                   lastFourUnshadowed = 0;

        if (i >= 4 && lastFourUnshadowed >= 4) break;
    }

    return shadow / max(total, 1.0);
}

float ShadowCalculation(vec3 fragWorldPos, vec3 normal, vec3 lightDir)
{
    int bestCascade = -1;

    vec3 cascadeProjCoords;

    float cascadeDepth = 0.0;

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

    if (bestCascade == -1)
        return 0.0; // not in any cascade, fully lit

    // Normal-offset shadows:
    // Move the receiver along the *geometric* normal, by ~N texels in world space
    float worldTexel = u_CascadeWorldTexelSize[bestCascade];
    vec3  offsetPos  = fragWorldPos + normal * (u_NormalOffsetScale * worldTexel);

    // Recompute light-space coords using the offset position
    vec4 lightSpacePosOffset = u_LightSpaceMatrices[bestCascade] * vec4(offsetPos, 1.0);
    vec3 projCoordsOffset = lightSpacePosOffset.xyz / lightSpacePosOffset.w * 0.5 + 0.5;

    cascadeProjCoords = projCoordsOffset;
    cascadeDepth = projCoordsOffset.z;

    // Slope-scaled depth bias (now as uniforms)
    float ndotl = max(dot(normal, lightDir), 0.0);
    float bias  = max(u_ShadowBiasSlope * (1.0 - ndotl), u_ShadowBiasMin);

    float shadowCascade = 0.0;
    shadowCascade = ComputeShadowPCSS(u_ShadowMaps[bestCascade], cascadeProjCoords, cascadeDepth, bias, bestCascade);

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

bool insideFrustumClip_MinusOneToOne(mat4 VP, vec3 worldPos)
{
    vec4 c = VP * vec4(worldPos, 1.0);
    if (c.w <= 0.0) return false; // behind camera
    return  abs(c.x) <= c.w &&
            abs(c.y) <= c.w &&
            c.z >= -c.w && c.z <= c.w;
}

void main()
{
//    // Debug: visualize cascades
//    // Is this fragment inside ANY shadow cascade?
//    int cascadeHit = -1;
//    for (int i = 0; i < u_NumCascades; ++i)
//    {
//        vec4 lpos = u_LightSpaceMatrices[i] * vec4(v_FragPos, 1.0);
//        vec3 pc = lpos.xyz / lpos.w * 0.5 + 0.5;
//        if (all(greaterThanEqual(pc, vec3(0.0))) &&
//            all(lessThanEqual (pc, vec3(1.0))))
//        {
//            cascadeHit = i;
//            break;
//        }
//    }
//
//    bool inActiveFrustum = insideFrustumClip_MinusOneToOne(u_DebugVP, v_FragPos);
//
//    // If the fragment is inside the ACTIVE camera's frustum, colour it:
//    //  - Blue   if it's also inside any shadow map cascade
//    //  - Purple if it's NOT inside a shadow map cascade
//    if (inActiveFrustum)
//    {
//        if (cascadeHit >= 0) {
//            FragColor = vec4(0.0, 0.0, 1.0, 1.0);   // blue
//        } else {
//            FragColor = vec4(1.0, 0.0, 1.0, 1.0);   // purple
//        }
//        return;
//    }
//
//    int bestCascade = -1;
//
//    for (int i = 0; i < u_NumCascades; ++i)
//    {
//        vec4 lightSpacePos = u_LightSpaceMatrices[i] * vec4(v_FragPos, 1.0);
//        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w * 0.5 + 0.5;
//
//        if (all(greaterThanEqual(projCoords, vec3(0.0))) &&
//            all(lessThanEqual (projCoords, vec3(1.0))))
//        {
//            bestCascade = i;
//            break;
//        }
//    }
//
//    if (bestCascade >= 0)
//    {
//        // Map 0->green, 1->yellow, 2->red (extra cascades: clamp to red)
//        vec3 debugCol =
//              (bestCascade == 0) ? vec3(0.0, 1.0, 0.0) :
//              (bestCascade == 1) ? vec3(1.0, 1.0, 0.0) :
//                                   vec3(1.0, 0.0, 0.0);
//
//        FragColor = vec4(debugCol, 1.0);
//        return;
//    }
//    // Debug visual ends

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
    float roughness = clamp(u_RoughnessFactor * roughnessTex, 0.02, 1.0);

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

    if (!gl_FrontFacing) N = N; // Trees look normal if we don't flip normals on backfaces (?)

    vec3 V = normalize(u_ViewPos - v_FragPos);
    vec3 L = normalize(-u_DirLightDirection);
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    // Base reflectance
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Cook-Torrance direct light

    // GGX / Trowbridge–Reitz NDF
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denomGGX = NdotH2 * (a2 - 1.0) + 1.0;
    float NDF = a2 / (3.14159 * denomGGX * denomGGX + 0.0001);

    // Smith GGX geometry, Heitz height-correlated form
    float Gv = NdotL * sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    float Gl = NdotV * sqrt(a2 + (1.0 - a2) * NdotL * NdotL);
    float G = (2.0 * NdotL * NdotV) / max(Gv + Gl, 1e-4);

    // Roughness aware Fresnel
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    float shadow = ShadowCalculation(v_FragPos, Ngeom, L);

    vec3 numerator = NDF * G * F;
    float denom = 4.0 * max(NdotV, 0.0) * NdotL + 0.001;
    vec3 specular = numerator / denom;

    // Some exporters put transmission only in the texture and leave factor = 0.
    // Read the texture first, then multiply.
    float tTex = u_HasTransmissionTex ? texture(u_TransmissionTex, v_TexCoord).r : 1.0;
    float transmission = clamp(u_TransmissionFactor * tTex, 0.0, 1.0);

    // Treat as "glass" if either factor or texture wants it.
    bool isGlass = (u_TransmissionFactor > 0.001) || (u_HasTransmissionTex && tTex > 0.001);

    // For glass: no diffuse
    if (isGlass) kD = vec3(0.0);

    vec3 radiance = u_DirLightColor * u_DirLightIntensity;

    // Burely/Disney diffuse
    float FD90 = 0.5 + 2.0 * roughness * pow(NdotH, 2.0);
    float Lm = (1.0 + (FD90 - 1.0) * pow(1.0 - NdotL, 5.0));
    float Vm = (1.0 + (FD90 - 1.0) * pow(1.0 - NdotV, 5.0));
    vec3 diffuse = kD * albedo * (Lm * Vm) / 3.14159;

    vec3 direct = (diffuse + specular) * radiance * NdotL * (1.0 - shadow);

    // IBL
    // Diffuse IBL: sample irradiance cube
    vec3 diffuseEnv = texture(u_IrradianceCube, N).rgb * u_EnvIntensity;
    vec3 diffuseIBL = diffuseEnv * (kD * albedo);

    // Specular IBL: prefiltered skybox via roughness-based LOD
    vec3 R = reflect(-V, N);
    float lod = roughness * u_PrefilterMaxLOD;
    vec3 envColor = textureLod(u_PrefilterEnv, R, lod).rgb * u_EnvIntensity;

    vec3 Fibl = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec2 brdf = texture(u_BRDFLUT, vec2(NdotV, roughness)).rg;

    vec3 specularIBL = envColor * (Fibl * brdf.x + brdf.y);

    float aoSpec = 1.0;
    float aoDiffuse = 1.0;
    if (u_UseSSAO)
    {
        // Sample SSAO and build attenuation terms
        vec2 aoUV = gl_FragCoord.xy * u_InvColorResolution;
        float ssao = texture(u_SSAO, aoUV).r;
        aoDiffuse = mix(1.0, max(ssao, u_AOMin), u_AOStrength);
        aoSpec = mix(1.0, sqrt(ssao), u_AOSpecScale);
    }

    vec3 ambient;
    if (isGlass)
    {
        // Thin-surface transmission via refracted IBL
        float eta  = max(u_IOR, 1.0001);
        vec3 Tdir = refract(-V, N, 1.0 / eta);
        vec3 envT = textureLod(u_PrefilterEnv, Tdir, lod).rgb;

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