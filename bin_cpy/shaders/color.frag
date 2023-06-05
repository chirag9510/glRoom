#version 460 core

layout (location = 1) out vec3 RGB32FColor;										//writing to TexHDR, hence why index = 1 and vec3

in VS_OUT
{
	vec3 Color;
}fs_in;
void main()
{
	RGB32FColor = fs_in.Color;
}