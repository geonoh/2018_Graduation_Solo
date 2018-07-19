#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;

in vec2 v_TexPos;

void main()
{
	vec4 color1 = texture(u_Texture0, v_TexPos);
	vec4 color2 = texture(u_Texture1, v_TexPos);
	FragColor = color1 + color2;
}
