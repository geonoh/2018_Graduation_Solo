#version 330

in vec3 a_Position;

out vec2 v_Pos;

void main()
{
	gl_Position = vec4(a_Position, 1);
	v_Pos = a_Position.xy;
}
