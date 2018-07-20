#version 330

uniform float u_Time;

in vec3 a_Position; in float a_Info;

void main()
{
	/*gl_Position 
	= 
	vec4(sin(u_Time)*0.5, cos(u_Time)*0.5, 0, 1);*/
	
	/*float newX = fract(u_Time)*2 - 1; //-1~1
	float newY = sin(newX * 3.141592 * 3);
	gl_Position = vec4(newX, newY, 0, 1);*/
		
	float newX = fract(u_Time)*2-1; //-1~1 
	float newY = sin((newX+1)*2*3.141592) * a_Info;
	gl_Position = vec4(newX, newY, 0, 1);
}
