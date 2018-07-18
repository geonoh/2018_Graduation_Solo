#version 330

layout(location=0) out vec4 FragColor;

uniform float u_Time;

uniform sampler2D u_TextureSlot; //twice
uniform sampler2D u_TextureSlot1; //bts

in vec2 v_TexPos;

void main()
{
	/*float tX = v_TexPos.x;
	float tY = v_TexPos.y;
	vec4 t1 = texture(u_TextureSlot, vec2(tX, tY)).bgra;
	vec4 t2 = texture(u_TextureSlot1, vec2(tX, tY)).bgra;

	float newX = v_TexPos.x * 3.141592 * 2;
	float newY = v_TexPos.y * 2 - 1;

	if(sin(newX) > newY)
		FragColor = t1;
	else
		FragColor = t2;

	float newnewY = v_TexPos.y*3.141592*50 + u_Time;
	float greyScale = sin(newnewY);
	FragColor = FragColor * abs(vec4(pow(greyScale, 7)));*/

	float repeat = 10;
	float sheer = 1;	                    
	float tX = fract(floor(v_TexPos.y * repeat)/sheer + fract(v_TexPos.x*repeat));
	float tY = fract(v_TexPos.y * repeat);
	vec4 t1 = texture(u_TextureSlot, vec2(tX, tY)).bgra;
	FragColor = t1;//vec4(tX, tY, 0, 1);
}
