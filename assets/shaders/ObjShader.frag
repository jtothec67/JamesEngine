#version 460

#define NUM_CASCADES 1
#define NUM_PREBAKED 4

#define MAX_IBL_LOD 5

uniform sampler2D u_Texture;
in vec2 v_TexCoord;

in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_ViewPos;
uniform vec3 u_Ambient;
uniform float u_SpecStrength;

uniform float u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;

uniform vec3 u_DirLightDirection;
uniform vec3 u_DirLightColor;

uniform sampler2D u_ShadowMaps[NUM_CASCADES];
uniform mat4 u_LightSpaceMatrices[NUM_CASCADES];

uniform sampler2D u_PreBakedShadowMaps[NUM_PREBAKED];
uniform mat4 u_PreBakedLightSpaceMatrices[NUM_PREBAKED];

uniform samplerCube u_SkyBox;

out vec4 FragColor;

const int NUM_POISSON_SAMPLES = 12;

vec2 poissonDisk[NUM_POISSON_SAMPLES] = vec2[](
    // 4 0uter samples (Up, Down, Left, Right)
    vec2(0.0,  1.5),
    vec2(0.0, -1.5),
    vec2(-1.5, 0.0),
    vec2(1.5,  0.0),

    // 8 inner semi-random samples
    vec2(-0.613,  0.245),
    vec2(0.535,  0.423),
    vec2(-0.245, -0.567),
    vec2(0.126,  0.946),
    vec2(-0.931,  0.081),
    vec2(0.804, -0.374),
    vec2(-0.321,  0.715),
    vec2(0.255, -0.803)
);

vec3 sampleOffsets[8] = vec3[](
    vec3( 0.0,  1.0,  0.0),
    vec3( 1.0,  0.0,  0.0),
    vec3( 0.0,  0.0,  1.0),
    vec3(-1.0,  0.0,  0.0),
    vec3( 0.0, -1.0,  0.0),
    vec3( 0.7, 0.7, 0.0),
    vec3( 0.0, 0.7, 0.7),
    vec3( 0.7, 0.0, 0.7)
);

float ComputeShadowPoissonPCF(sampler2D shadowMap, vec3 projCoords, float depth, float bias)
{
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;
    float totalSamples = 0.0;

    ivec2 screenCoord = ivec2(gl_FragCoord.xy);

    // Per-pixel rotation (keeps pattern varied)
    float angle = fract(sin(dot(vec2(screenCoord), vec2(12.9898, 78.233))) * 43758.5453) * 6.2831;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    // --- Initial Early-Out with Outer 4 Samples (indices 0–3) ---
    int earlyShadowCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        vec2 offset = rotation * poissonDisk[i] * texelSize;
        float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
        if (depth - bias > sampleDepth)
            earlyShadowCount++;
    }

    if (earlyShadowCount == 0)
        return 0.0; // Fully lit
    else if (earlyShadowCount == 4)
        return 1.0; // Fully shadowed

    // --- Rolling PCF Loop (indices 0–11) ---
    int lastFourUnshadowed = 0;

    for (int i = 0; i < NUM_POISSON_SAMPLES; ++i)
    {
        vec2 offset = rotation * poissonDisk[i] * texelSize;
        float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
        float isShadowed = (depth - bias > sampleDepth) ? 1.0 : 0.0;

        shadow += isShadowed;
        totalSamples += 1.0;

        if (isShadowed == 0.0)
            lastFourUnshadowed++;
        else
            lastFourUnshadowed = 0;

        // Rolling early-out: last 4 samples were all lit
        if (i >= 4 && lastFourUnshadowed >= 4)
            break;
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

    // Check pre-baked shadows
    for (int i = 0; i < NUM_PREBAKED; ++i)
    {
        vec4 lightSpacePos = u_PreBakedLightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w * 0.5 + 0.5;

        if (all(greaterThanEqual(projCoords, vec3(0.0))) && all(lessThanEqual(projCoords, vec3(1.0))))
        {
            bestPrebaked = i;
            prebakedProjCoords = projCoords;
            prebakedDepth = projCoords.z;
            break;
        }
    }

    // Check cascade shadows
    for (int i = 0; i < NUM_CASCADES; ++i)
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
    float shadowPrebaked = 0.0;

    if (bestCascade != -1)
    {
        shadowCascade = ComputeShadowPoissonPCF(u_ShadowMaps[bestCascade], cascadeProjCoords, cascadeDepth, bias);
    }

    if (bestPrebaked != -1)
    {
        shadowPrebaked = ComputeShadowPoissonPCF(u_PreBakedShadowMaps[bestPrebaked], prebakedProjCoords, prebakedDepth, bias);
    }

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


void main()
{
    vec4 tex = texture(u_Texture, v_TexCoord);
    vec3 albedo = tex.rgb * u_BaseColorFactor;
    float metallic = clamp(u_MetallicFactor, 0.0, 1.0);
    float roughness = clamp(u_RoughnessFactor, 0.04, 1.0);
    float ao = 1.0;

    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_ViewPos - v_FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 L = normalize(-u_DirLightDirection);
    vec3 H = normalize(V + L);
    vec3 radiance = u_DirLightColor;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    float shadow = ShadowCalculation(v_FragPos, N, L);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    vec3 direct = (kD * albedo / 3.14159 + specular) * radiance * NdotL * (1.0 - shadow);

    vec3 R = reflect(-V, N);
    const int SAMPLE_COUNT = 8;
    float roughnessClamped = clamp(roughness, 0.01, 1.0);

    vec3 blurredReflection = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        // Use a few offset directions (hardcoded or from a lookup table)
        vec3 offset = normalize(R + roughnessClamped * (sampleOffsets[i] * 10));
        float weight = 1.0; // could weight based on angle/distance if you want

        blurredReflection += texture(u_SkyBox, offset).rgb * weight;
        totalWeight += weight;
    }

    blurredReflection /= totalWeight;

    vec3 specularIBL = blurredReflection * F * (1.0 - shadow) * 1.5;

    vec3 ambient = u_Ambient * (kD * albedo + specularIBL) * ao;

    vec3 color = ambient + direct;
    FragColor = vec4(color, tex.a);
}
