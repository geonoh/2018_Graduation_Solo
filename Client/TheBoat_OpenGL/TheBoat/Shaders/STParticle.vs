#version 330

in vec3 a_Position;

uniform float u_Time;
uniform vec2 u_S;
uniform vec2 u_E;

void main()
{
	float pi = 3.141592;
	vec2 newPos = vec2(-10, -10);
	vec2 k = u_E - u_S;
	mat2 rot = mat2(cos(pi/2), sin(pi/2), -sin(pi/2), cos(pi/2));
	vec2 verti = normalize(rot*k);

	if(u_Time > a_Position.x)
	{
		float newTime = u_Time - a_Position.x;
		newTime = fract(newTime/4)*2;
		float temp = sin(newTime*3.141592*a_Position.y*3) * a_Position.z * 0.3;
		newPos = u_S + k*newTime;
		newPos += verti*temp;
	}

	gl_Position = vec4
	(
		newPos.x, //-1 -- 1
		newPos.y,
		0,
		1
	);

	gl_PointSize = 50;
}
