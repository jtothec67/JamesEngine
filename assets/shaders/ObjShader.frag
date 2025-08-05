#version 460

#define MAX_LIGHTS 10
#define NUM_CASCADES 4
#define NUM_PREBAKED 4

uniform sampler2D u_Texture;
in vec2 v_TexCoord;

in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_ViewPos;
uniform vec3 u_Ambient;
uniform float u_SpecStrength;

uniform vec3 u_LightPositions[MAX_LIGHTS];
uniform vec3 u_LightColors[MAX_LIGHTS];
uniform float u_LightStrengths[MAX_LIGHTS];

uniform vec3 u_DirLightDirection;
uniform vec3 u_DirLightColor;

uniform sampler2D u_ShadowMaps[NUM_CASCADES];
uniform mat4 u_LightSpaceMatrices[NUM_CASCADES];

uniform sampler2D u_PreBakedShadowMaps[NUM_PREBAKED];
uniform mat4 u_PreBakedLightSpaceMatrices[NUM_PREBAKED];

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


void main()
{
    vec4 tex = texture(u_Texture, v_TexCoord);

    vec3 N = normalize(v_Normal);
    vec3 viewDir = normalize(u_ViewPos - v_FragPos);
    vec3 lighting = u_Ambient;

    // Point lights
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        vec3 lightDir = normalize(u_LightPositions[i] - v_FragPos);
        float diff = max(dot(N, lightDir), 0.0);
        vec3 diffuse = u_LightColors[i] * diff * u_LightStrengths[i];

        vec3 reflectDir = reflect(-lightDir, N);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
        vec3 specular = spec * u_LightColors[i] * u_LightStrengths[i] * u_SpecStrength;

        lighting += diffuse + specular;
    }

    // Directional light
    vec3 dirLightDir = normalize(-u_DirLightDirection);
    float diff = max(dot(N, dirLightDir), 0.0);
    vec3 diffuse = u_DirLightColor * diff;

    vec3 reflectDir = reflect(-dirLightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = spec * u_DirLightColor * u_SpecStrength;

    float shadow = ShadowCalculation(v_FragPos, N, dirLightDir);

    lighting += (1.0 - shadow) * (diffuse + specular);

    FragColor = vec4(lighting, 1.0) * tex;
}
