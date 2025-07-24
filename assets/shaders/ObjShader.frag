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

float ShadowCalculation(vec3 fragWorldPos, vec3 normal, vec3 lightDir)
{
    //if (NUM_CASCADES + NUM_PREBAKED == 0)
        //return 0.0;

    int bestCascade = -1;
    int bestPrebaked = -1;

    vec3 cascadeProjCoords;
    vec3 prebakedProjCoords;

    float cascadeDepth = 0.0;
    float prebakedDepth = 0.0;

    float bias = max(0.003 * (1.0 - dot(normal, lightDir)), 0.0005);

    // Check prebaked
    for (int i = 0; i < NUM_PREBAKED; ++i)
    {
        vec4 lightSpacePos = u_PreBakedLightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        projCoords = projCoords * 0.5 + 0.5;

        if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
            projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
            projCoords.z >= 0.0 && projCoords.z <= 1.0)
        {
            bestPrebaked = i;
            prebakedProjCoords = projCoords;
            prebakedDepth = projCoords.z;
            break;
        }
    }

    // Check cascades
    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        vec4 lightSpacePos = u_LightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        projCoords = projCoords * 0.5 + 0.5;

        if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
            projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
            projCoords.z >= 0.0 && projCoords.z <= 1.0)
        {
            bestCascade = i;
            cascadeProjCoords = projCoords;
            cascadeDepth = projCoords.z;
            break;
        }
    }

     float shadowCascade = 0.0;
    float shadowPrebaked = 0.0;

    // Early-out for cascade shadows
    if (bestCascade != -1)
    {
        // Check center sample first
        float centerDepth = texture(u_ShadowMaps[bestCascade], cascadeProjCoords.xy).r;
        
        // If fragment is definitely not in shadow (with generous bias), skip expensive PCF
        if (cascadeDepth - bias < centerDepth)
        {
            // Not in shadow, skip PCF
            shadowCascade = 0.0;
        }
        else
        {
            // Might be in shadow, do PCF
            vec2 texelSize = 1.0 / textureSize(u_ShadowMaps[bestCascade], 0);
            
            // Enhanced PCF - use 5x5 for better quality
            for (int x = -2; x <= 2; ++x)
            {
                for (int y = -2; y <= 2; ++y)
                {
                    vec2 offset = vec2(x, y) * texelSize;
                    float pcfDepth = texture(u_ShadowMaps[bestCascade], cascadeProjCoords.xy + offset).r;
                    shadowCascade += cascadeDepth - bias > pcfDepth ? 1.0 : 0.0;
                }
            }
            shadowCascade /= 25.0; // 5x5 samples
        }
    }

    // Early-out for prebaked shadows
    if (bestPrebaked != -1)
    {
        // Check center sample first
        float centerDepth = texture(u_PreBakedShadowMaps[bestPrebaked], prebakedProjCoords.xy).r;
        
        // If fragment is definitely not in shadow (with generous bias), skip expensive PCF
        if (prebakedDepth - bias < centerDepth)
        {
            // Not in shadow, skip PCF
            shadowPrebaked = 0.0;
        }
        else
        {
            // Might be in shadow, do PCF
            vec2 texelSize = 1.0 / textureSize(u_PreBakedShadowMaps[bestPrebaked], 0);
            
            // Enhanced PCF - use 5x5 for better quality
            for (int x = -2; x <= 2; ++x)
            {
                for (int y = -2; y <= 2; ++y)
                {
                    vec2 offset = vec2(x, y) * texelSize;
                    float pcfDepth = texture(u_PreBakedShadowMaps[bestPrebaked], prebakedProjCoords.xy + offset).r;
                    shadowPrebaked += prebakedDepth - bias > pcfDepth ? 1.0 : 0.0;
                }
            }
            shadowPrebaked /= 25.0; // 5x5 samples
        }
    }

    float shadow = max(shadowCascade, shadowPrebaked);
    shadow = pow(shadow, 2.0);

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
