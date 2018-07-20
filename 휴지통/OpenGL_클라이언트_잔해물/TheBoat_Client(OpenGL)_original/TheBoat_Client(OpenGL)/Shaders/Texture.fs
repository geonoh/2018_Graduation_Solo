#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_TextureSlot;

in vec2 v_TexPos;

void main()
{
	//FragColor = vec4(fract(v_TexPos * 3), 0, 1);
	FragColor = texture(u_TextureSlot, v_TexPos);
}
