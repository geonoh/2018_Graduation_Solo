#version 330

layout(location=0) out vec4 FragColor;
const float pi = 3.141592;
in vec2 v_Pos;

void main()
{
	/*float distance = length(v_Pos); // 0~1
	float greyScale = sin(distance*4*pi); //0~4pi
	FragColor = vec4(greyScale);*/
	
	/*float distance = length(v_Pos); // 0~1
	if(distance < 1)
		FragColor = vec4(1);
	else
		FragColor = vec4(0);*/
		
	float distance1 = length(v_Pos-vec2(-1, 0)); // 0~1
	float distance2 = length(v_Pos-vec2(1, 0)); // 0~1
	float greyScale1 = clamp(-(floor(distance1)-1), 0, 1);
	float greyScale2 = clamp(-(floor(distance2)-1), 0, 1);
	FragColor = vec4(greyScale1+greyScale2);
}
