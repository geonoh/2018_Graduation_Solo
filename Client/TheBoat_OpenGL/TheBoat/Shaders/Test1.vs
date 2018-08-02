#version 330

in vec3 a_Position;
in vec2 a_TexPos;

uniform vec2 u_Trans;
uniform vec2 u_Scale;

out vec2 v_TexPos;

void main()
{
	vec3 newPos = a_Position.xyz;
	newPos.xy *= u_Scale;
	newPos.xy += u_Trans; //translation
	gl_Position = vec4(newPos.xyz, 1);
	v_TexPos = a_TexPos;
}
