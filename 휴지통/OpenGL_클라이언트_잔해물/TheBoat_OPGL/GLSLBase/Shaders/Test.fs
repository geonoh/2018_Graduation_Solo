#version 330

layout(location=0) out vec4 FragColor;

in vec2 v_Pos;
const float pi = 3.141592;

uniform float u_Time;
uniform vec2 u_Centers[4];

void main()
{
	FragColor = vec4(0);

	for(int i=0; i<4; i++)
	{
		vec2 pos = v_Pos;
		vec2 center = u_Centers[i];
		float distance = length(pos-center) * 2 * pi;

		FragColor += vec4(sin(distance*10 - u_Time)*0.2);
	}
}
