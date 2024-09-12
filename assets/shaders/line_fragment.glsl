#version 460 core

uniform int u_color;
out vec4 color;

void main()
{
	color = vec4((u_color & 0xFF) / 255.0F, ((u_color >> 8) & 0xFF) / 255.0F, ((u_color >> 16) & 0xFF) / 255.0F, 1.0);
}