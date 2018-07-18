#version 330

in vec3 a_Position;

out float v_Color;
out vec2 v_Tex;

uniform float u_Time;
uniform mat4 u_ProjView;

void main()
{
	float newX = a_Position.x;
	float newY = a_Position.y; 
	float newZ = a_Position.z 
	+ 0.5*sin(u_Time + (newX+0.5)*2*3.141592);
	
	gl_Position = u_ProjView * vec4(newX, newY, newZ, 1);

	v_Color = (1+sin(u_Time + (newX+0.5)*2*3.141592))/2;

	v_Tex = a_Position.xy + vec2(0.5, 0.5); //0~1
}
