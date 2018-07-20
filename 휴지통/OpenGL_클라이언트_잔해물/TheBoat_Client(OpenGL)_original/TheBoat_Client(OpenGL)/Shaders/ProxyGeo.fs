#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_Tex;

in float v_Color;
in vec2 v_Tex;

void main()
{
	FragColor = texture(u_Tex, v_Tex).bgra;
	//FragColor = vec4(v_Tex, 0, 1);
	//FragColor = vec4(v_Color);
}
