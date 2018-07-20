#version 330

in vec3 a_Position;

out vec3 v_TexCoord;

uniform mat4 u_PVM;

void main()
{
	gl_Position = u_PVM * vec4(a_Position, 1);
	v_TexCoord = a_Position;
}
