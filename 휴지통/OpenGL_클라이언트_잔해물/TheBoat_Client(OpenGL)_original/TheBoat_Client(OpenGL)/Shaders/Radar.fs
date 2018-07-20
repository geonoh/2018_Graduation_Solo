#version 330

layout(location=0) out vec4 FragColor;

in vec2 v_Pos;

const float pi = 3.141592;

uniform float u_Time;
uniform vec2 u_Centers[4];

void main()
{
	float weight = 0;

	for(int i=0; i<4; i++)
	{
		float distance = length(v_Pos - u_Centers[i]);
		if(distance < 0.05)
			weight += 0.3;
	}

	float distance1 = length(v_Pos);
	float newTime = fract(u_Time/2)*2;
	float finalColor = 0;
	if(distance1 > newTime - 0.02 && distance1 < newTime + 0.02)
	{
		finalColor = 1 * weight + 0.2;
	}
	else
	{
		discard;
	}
	
	FragColor = vec4(finalColor);
}
