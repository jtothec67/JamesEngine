#version 460

uniform sampler2D u_Texture;
in vec2 v_TexCoord;

in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_ViewPos;
uniform vec3 u_Ambient;
uniform float u_SpecStrength;

#define MAX_LIGHTS 10
uniform vec3 u_LightPositions[MAX_LIGHTS];
uniform vec3 u_LightColors[MAX_LIGHTS];
uniform float u_LightStrengths[MAX_LIGHTS];

uniform vec3 u_DirLightDirection;
uniform vec3 u_DirLightColor;

uniform sampler2D u_ShadowMap;
uniform mat4 u_LightSpaceMatrix;

out vec4 FragColor;

// --- Shadow calculation ---
float ShadowCalculation(vec3 fragWorldPos, vec3 normal, vec3 lightDir)
{
    vec4 lightSpacePos = u_LightSpaceMatrix * vec4(fragWorldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float closestDepth = texture(u_ShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Dynamic bias based on angle between normal and light direction
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    // PCF (simple 3x3 average)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
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
    for (int i = 0; i < u_LightPositions.length(); ++i)
    {
        vec3 lightDir = normalize(u_LightPositions[i] - v_FragPos);
        float diff = max(dot(N, lightDir), 0.0);
        vec3 diffuse = u_LightColors[i] * diff * u_LightStrengths[i];

        vec3 reflectDir = reflect(-lightDir, N);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
        vec3 specular = spec * u_LightColors[i] * u_LightStrengths[i] * u_SpecStrength;

        lighting += diffuse + specular;
    }

    // Directional light with shadow
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
