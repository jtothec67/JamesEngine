

uniform sampler2D u_Texture;
varying vec2 v_TexCoord;

varying vec3 v_Normal;
varying vec3 v_FragPos;

uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;
uniform vec3 u_Ambient;
uniform bool u_Specular;
uniform float u_LightStrength;

void main()
{
	vec4 tex = texture2D(u_Texture, v_TexCoord);

	vec3 diffuseColor = vec3(1, 1, 1);

	vec3 N = normalize(v_Normal);
	vec3 lightDir = normalize(u_LightPos - v_FragPos);
	float diff = max(dot(N, lightDir), 0.0);
	vec3 diffuse = diffuseColor * diff * u_LightStrength;

	vec3 specular = vec3(0, 0, 0);

	if (u_Specular)
	{
		vec3 specularColor = vec3(1, 1, 1);
		vec3 viewDir = normalize(u_ViewPos - v_FragPos);
		vec3 reflectDir = reflect(-lightDir, N);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		vec3 specular = spec * specularColor * u_LightStrength;
	}

	vec3 lighting = diffuse + specular + u_Ambient; 
	gl_FragColor =  vec4(lighting, 1) * tex;
}