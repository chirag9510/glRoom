#version 460 core
#extension GL_ARB_bindless_texture : require

 //hi res HDR fbo stores result in vec3 since its GL_RGB16F, only rgb no alpha hence vec3
layout (location = 0) out vec4 fragColor;							//last step used by default fbo etc, must be at index 0
layout (location = 1) out vec3 RGB16FColor;							//used by fbos for their first default output of color data to write to their attached color buffer GL_COLOR_ATTACHMENT0 texture

layout (binding=0) uniform sampler2D TexHDR;
layout (binding=1) uniform sampler2D TexBlur1;
layout (binding=2) uniform sampler2D TexBlur2;
layout (binding=3) uniform sampler2D TexModel;						//sampler used by some non indirect meshes

layout(binding = 2, std430) readonly buffer TextureHandles
{
	sampler2D samplerBindlessTex[];
}textureHandles;

layout(binding = 3, std430) readonly buffer KdColors
{
	vec4 color[];
}kdColors;

layout(binding = 4, std430) readonly buffer GaussWeights
{
	float weights[];
}gaussWeights;

in VS_OUT
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
	flat int DrawID;
}fs_in;

subroutine vec3 RenderType();
subroutine uniform RenderType renderType;

uniform int Pass = 1;
float MaxWhiteLum = .9f;

//PASS 1 Rendering scene, store result hi res HDR fbo
vec3 calculateADS()
{
	//ads
	vec3 n = normalize(fs_in.Normal);
	vec3 v = normalize(-fs_in.Position);							
	vec3 h = normalize(2 * v);											//directional light follows view

	return vec3(0.1f, 0.05f, 0.05f) + 
				vec3(.75f, .75f, .55f) * max(dot(v, n), 0.f) + 
				vec3(.4f, .4f, .4f) * pow(max(dot(h, n), 0.f), 8.f);
}
subroutine(RenderType)
vec3 basicKd()
{
    //non textured mesh using color
	return calculateADS() * kdColors.color[fs_in.DrawID].rgb;
}
subroutine(RenderType)
vec3 textured()
{
	return calculateADS() * texture(textureHandles.samplerBindlessTex[fs_in.DrawID], fs_in.TexCoord).rgb;
}
subroutine(RenderType)
vec3 emissive()
{
	return texture(textureHandles.samplerBindlessTex[fs_in.DrawID], fs_in.TexCoord).rgb;
}
subroutine(RenderType)
vec3 basicEmissive()
{
    //non textured mesh using emissive property
	//only used by room
	return vec3(.015f, 0.f, 0.04f);
}

//Render gaussian blur thru brigh pass to blurpass textures, read TexHDR and bright pass write to TexBlur1					
float luminance( vec3 color )
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722)); 
}

subroutine(RenderType)
vec3 brightPass()
{
	vec3 colorHDR = texture(TexHDR,  fs_in.TexCoord).rgb;
	if(luminance(colorHDR) > .77f)										//.77 is LumThreshold
		return colorHDR;												//write to blurTex1 thru index 1
	else
		return vec3(0.f);
}
//read TexBlur1 and write to TexBlur2
subroutine(RenderType)
vec3 blurPassVertical()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec4 sum = texelFetch(TexBlur1, pixel, 0) * gaussWeights.weights[0];					//0 index is current pixel
	for(int i = 1; i < 5; i++)
	{
		sum += texelFetchOffset(TexBlur1, pixel, 0, ivec2(0, i)) * gaussWeights.weights[i];
		sum += texelFetchOffset(TexBlur1, pixel, 0, ivec2(0, -i)) * gaussWeights.weights[i];
	}
	return sum.rgb;
}
//read TexBlur2 and write to TexBlur2
subroutine(RenderType)
vec3 blurPassHorizontal()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec4 sum = texelFetch(TexBlur2, pixel, 0) * gaussWeights.weights[0];				//0 index is current pixel
	for(int i = 1; i < 5; i++)
	{
		sum += texelFetchOffset(TexBlur2, pixel, 0, ivec2(i, 0)) * gaussWeights.weights[i];
		sum += texelFetchOffset(TexBlur2, pixel, 0, ivec2(-i, 0)) * gaussWeights.weights[i];
	}
	return sum.rgb;
}

//PASS 2 reinhard extended, combine with blurpass tex and output vec4
vec3 changeLuminance(vec3 colorIn, float lumOut)
{
    float lumIn = luminance(colorIn);
    return colorIn * (lumOut / lumIn);
}
vec4 reinhardExtendedComposite()
{
	//additive blend HDR and blur colors
	vec3 colorHDR = texture(TexHDR,  fs_in.TexCoord).rgb;
	colorHDR += texture(TexBlur1,  fs_in.TexCoord).rgb;
	
	float lumOld = luminance(colorHDR);
	float numerator = lumOld * (1.f + lumOld / (MaxWhiteLum * MaxWhiteLum));
	float lumNew = numerator / (1.f + lumOld);

	//gamma correct image and add with texBlur for bloom
	return vec4(pow(changeLuminance(colorHDR, lumNew), vec3(1.0 / 1.28f)), 1.0);						//1.2f is Gamma
}

void main()	
{
    if(Pass == 1)
		RGB16FColor = renderType();
	else if(Pass == 2)
    	fragColor = reinhardExtendedComposite();
}