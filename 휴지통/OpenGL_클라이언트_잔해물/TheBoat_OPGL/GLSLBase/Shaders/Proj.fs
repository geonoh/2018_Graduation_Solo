#version 330

uniform vec3 u_LightPos;
uniform vec3 u_CameraPos;
uniform vec3 u_Emissive;

const float c_Ambient = 0.2;
const float c_Diffuse = 0.7;
const float c_Specular = 0.7;

in vec4 v_Color;
in vec3 v_Normal;
in vec3 v_Pos;

layout(location=0) out vec4 FragColor;
layout(location=1) out vec4 FragEmissive;

void main()
{
	vec3 lightDir = normalize(u_LightPos - v_Pos);
	vec3 viewDir = normalize(u_CameraPos - v_Pos);
	vec3 reflectDir = reflect(-lightDir, v_Normal);

	float ambient = 1.0;
	float diffuse = max(0, dot(lightDir, v_Normal));
	float specular = pow(max(0, dot(viewDir, reflectDir)), 500);
	
    FragColor.rgb = 	
		(c_Ambient*ambient +
		c_Diffuse*diffuse +
		c_Specular*specular +
		u_Emissive) * v_Color.rgb;
	FragColor.a = v_Color.a;

	vec3 emissive = max(vec3(0, 0, 0), FragColor.rgb - vec3(1, 1, 1));
	FragEmissive = vec4(emissive, 1);
}
