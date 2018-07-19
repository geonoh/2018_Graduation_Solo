#version 330

layout(location=0) out vec4 FragColor;

uniform sampler2D u_TextureSlot;

in vec2 v_TexPos;

void main()
{
	vec2 tex_Offset = 2.0 / textureSize(u_TextureSlot, 0);

	vec3 baseColor = vec3(0);//texture(u_TextureSlot, v_TexPos).rgb;

	float weight = 0.05;

	for(int i=0; i<30; i++)
	{
		//x Axis
		vec2 newTexPos = v_TexPos + vec2(tex_Offset.x*i, 0);
		vec4 color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;

		newTexPos = v_TexPos - vec2(tex_Offset.x*i, 0);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;
		
		//y Axis
		newTexPos = v_TexPos + vec2(0, tex_Offset.y*i);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;

		newTexPos = v_TexPos - vec2(0, tex_Offset.y*i);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;
		
		float rt2 = sqrt(2.0);
		//xy Axis
		newTexPos = v_TexPos + rt2*vec2(tex_Offset.x*i, tex_Offset.y*i);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;

		newTexPos = v_TexPos - rt2*vec2(tex_Offset.x*i, tex_Offset.y*i);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;
		
		//-xy Axis
		newTexPos = v_TexPos + rt2*vec2(-tex_Offset.x*i, tex_Offset.y*i);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;

		newTexPos = v_TexPos - rt2*vec2(-tex_Offset.x*i, tex_Offset.y*i);
		color = texture(u_TextureSlot, newTexPos)*weight;
		baseColor += color.rgb;
	}

	FragColor = vec4(baseColor, 1);
}

/*
in vec2 v_TexPos;

uniform sampler2D u_Texture;

layout(location=0) out vec4 FragColor;

uniform bool horizontal = true;;
uniform float weight[7] = float[] (0.12344, 0.20700, 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 2.0 / textureSize(u_Texture, 0); // gets size of single texel
    vec3 result = texture(u_Texture, v_TexPos).rgb * weight[0]; // current fragment's contribution

    for(int i = 1; i < 28; ++i)
    {
        result += texture(u_Texture, v_TexPos + vec2(tex_offset.x * i, 0.0)).rgb * weight[i/4];
        result += texture(u_Texture, v_TexPos - vec2(tex_offset.x * i, 0.0)).rgb * weight[i/4];
    }

    for(int i = 1; i < 28; ++i)
    {
        result += texture(u_Texture, v_TexPos + vec2(0.0, tex_offset.y * i)).rgb * weight[i/4];
        result += texture(u_Texture, v_TexPos - vec2(0.0, tex_offset.y * i)).rgb * weight[i/4];
    }

    FragColor = vec4(result, 1.0);
}*/