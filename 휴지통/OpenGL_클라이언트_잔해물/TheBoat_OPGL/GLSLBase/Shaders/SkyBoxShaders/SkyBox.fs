#version 330

layout(location=0) out vec4 FragColor;

uniform samplerCube u_CubeTexture;

in vec3 v_TexCoord;

void main()
{
	FragColor = texture(u_CubeTexture, v_TexCoord);
}
