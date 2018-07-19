#version 330

in vec4 a_Position;
out vec2 v_Pos;
void main()
{
	gl_Position = vec4(a_Position.xyz, 1);
	
	v_Pos = a_Position.xy;
}
