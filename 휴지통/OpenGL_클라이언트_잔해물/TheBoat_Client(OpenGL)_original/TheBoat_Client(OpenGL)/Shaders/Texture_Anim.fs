#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_TextureSlot;

in vec2 v_TexPos;

uniform float u_AnimStep; //0, 1, 2, 3, 4, 5

void main()
{
	float tX = v_TexPos.x; //0~1 -> 0~1
	float tY = u_AnimStep/6 + v_TexPos.y/6; //0~1 -> 0~1/6
	FragColor = texture(u_TextureSlot, vec2(tX, tY));
}
