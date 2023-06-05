#version 460 core
#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;

layout (binding = 0, std140) uniform PersepctiveMatrices
{
	mat4 matView;
	mat4 matProj;
}persMatrices;

layout (binding = 0, std430) readonly buffer Transforms
{
	mat4 matModel[];
}transforms;

out VS_OUT
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
	flat int DrawID;
}vs_out;

void main()
{
	mat4 matModel = transforms.matModel[gl_BaseInstance + gl_InstanceID];
	mat4 matModelView = persMatrices.matView * matModel;
	gl_Position = persMatrices.matProj * matModelView * vec4(aPos, 1.f);
	vs_out.TexCoord = aTex;
	vs_out.Normal = (matModelView * vec4(aNormal, 0.f)).xyz;
	vs_out.Position = (matModelView * vec4(aPos, 1.f)).xyz;
	vs_out.DrawID = gl_DrawID;
}