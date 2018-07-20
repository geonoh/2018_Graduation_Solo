#version 330

in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_Tex;

uniform mat4 u_ProjView;
uniform mat4 u_Model;
uniform mat4 u_Rotation;

out vec3 v_Pos;
out vec3 v_Normal;
out vec2 v_Tex;

void main()
{
  gl_Position = u_ProjView * vec4(a_Position, 1.f);
  v_Pos = a_Position;
  v_Normal = a_Normal;
  v_Tex = a_Tex;

  /*gl_Position = u_ProjView * u_Model * vec4(a_Position, 1.f);
  v_Pos = (u_Model * vec4(a_Position, 1.f)).xyz;
  v_Normal = (u_Rotation * vec4(a_Normal, 0.f)).xyz;
  v_Tex = a_Tex;*/

}
