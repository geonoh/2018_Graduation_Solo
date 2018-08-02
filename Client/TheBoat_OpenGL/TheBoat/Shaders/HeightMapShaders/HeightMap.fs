#version 330

uniform vec3 u_LightPos;
uniform vec3 u_CameraPos;
uniform vec3 u_Emissive;

uniform sampler2D u_TextureBase;
uniform sampler2D u_TextureDetail;

const float c_Ambient = 0.8;
const float c_Diffuse = 0.2;
const float c_Specular = 0.0;

in vec3 v_Pos;
in vec3 v_Normal;
in vec2 v_Tex;

layout(location=0) out vec4 FragColor;
layout(location=1) out vec4 FragEmissive;

void main()
{
	//vec3 lightDir = normalize(u_LightPos - v_Pos);
	vec3 lightDir = vec3(1, 0, 0);
	vec3 viewDir = normalize(u_CameraPos - v_Pos);
	vec3 reflectDir = reflect(-lightDir, v_Normal);

	float ambient = 1.0;
	float diffuse = max(0, dot(lightDir, v_Normal));
	float specular = pow(max(0, dot(viewDir, reflectDir)), 500);
	
    FragColor.rgb = 	
		(c_Ambient*ambient +
		c_Diffuse*diffuse +
		c_Specular*specular +
		u_Emissive);
	FragColor.a = 1.0;

    vec4 baseTex = texture(u_TextureBase, v_Tex);
    vec4 detailTex = texture(u_TextureDetail, v_Tex*50.0);

    FragColor *= (0.8*baseTex + 0.2*detailTex);

	vec3 emissive = max(vec3(0, 0, 0), FragColor.rgb - vec3(1, 1, 1));
	FragEmissive = vec4(emissive, 1);
}
