//useful for debugging
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (binding = 0, std140) uniform PersepctiveMatrices
{
	mat4 matView;
	mat4 matProj;
}persMatrices;

out VS_OUT
{
	vec3 Color;
}vs_out;

void main()
{
	gl_Position = persMatrices.matProj * persMatrices.matView * vec4(aPos, 1.f);
	vs_out.Color = aColor;
}