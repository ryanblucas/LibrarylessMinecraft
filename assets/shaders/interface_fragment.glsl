#version 460 core

in vec2 tex_pos;
flat in int tint;
out vec4 color;
uniform sampler2D sampler;

void main()
{
	color = vec4((tint & 0xFF) / 255.0F, ((tint >> 8) & 0xFF) / 255.0F, ((tint >> 16) & 0xFF) / 255.0F, 1.0) * texture(sampler, tex_pos);
}