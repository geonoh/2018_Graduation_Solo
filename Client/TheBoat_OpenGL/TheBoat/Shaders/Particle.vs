#version 330

in vec3 a_Position;
in vec2 a_TexPos;
in vec4 a_Velocity;

uniform mat4 u_ProjView;
uniform mat4 u_Rotation;

uniform float u_Time;

out vec2 v_TexPos;
out float v_Alpha;

const vec3 g_Gravity = vec3(-2, -100.5, 0);

void main()
{
   float newTime = u_Time - a_Velocity.w;
   vec3 newPos = vec3(-1000, -1000, -1000);
   float alpha = 0;
   if(newTime > 0)
   {
      newTime = fract(newTime/2)*2;
      alpha = sin(fract(newTime/2)*3.141592);
      newPos
       = 
       a_Position.xyz + //base position
       newTime*a_Velocity.xyz + // velocity*t
       0.5*g_Gravity.xyz*newTime*newTime; // acceleration
   }
   
    gl_Position = u_ProjView * vec4(newPos.xyz, 1.f);
   //gl_Position = vec4(newPos.xyz, 1);
   v_TexPos = a_TexPos;
   v_Alpha = alpha;
}