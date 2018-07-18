#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_Texture;

void main()
{
	//FragColor = vec4(gl_PointCoord, 0, 1);
	FragColor = texture(u_Texture, gl_PointCoord);
}
