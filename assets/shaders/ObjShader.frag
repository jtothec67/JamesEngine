#version 460

#define MAX_LIGHTS 10
#define NUM_CASCADES 4

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

out vec4 FragColor;

float ShadowCalculation(vec3 fragWorldPos, vec3 normal, vec3 lightDir)
{
    int bestCascade = -1;
    float bestTexelSize = 1e10;

    vec3 bestProjCoords;
    float bestCurrentDepth = 0.0;

    // Loop through all cascades to find valid ones
    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        vec4 lightSpacePos = u_LightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        projCoords = projCoords * 0.5 + 0.5;

        // Is the fragment inside this cascade's shadow map?
        if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
            projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
            projCoords.z >= 0.0 && projCoords.z <= 1.0)
        {
            float texelSize = 1.0 / float(textureSize(u_ShadowMaps[i], 0).x); // Assume square
            if (texelSize < bestTexelSize)
            {
                bestCascade = i;
                bestTexelSize = texelSize;
                bestProjCoords = projCoords;
                bestCurrentDepth = projCoords.z;
            }
        }
    }

    if (bestCascade == -1)
        return 0.0; // Not in any cascade bounds - fully lit

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;

    vec2 texelSizeVec = 1.0 / textureSize(u_ShadowMaps[bestCascade], 0);

    // 3x3 PCF
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offset = vec2(x, y) * texelSizeVec;
            float pcfDepth = texture(u_ShadowMaps[bestCascade], bestProjCoords.xy + offset).r;
            shadow += bestCurrentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

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
