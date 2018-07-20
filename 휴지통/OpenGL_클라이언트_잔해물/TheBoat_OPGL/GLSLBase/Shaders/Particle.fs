#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_Texture;

in vec2 v_TexPos;
in float v_Alpha;

void main()
{
	FragColor = texture(u_Texture, v_TexPos);
	FragColor.a = FragColor.a*v_Alpha;
}
