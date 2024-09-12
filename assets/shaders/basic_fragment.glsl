#version 460 core

in vec2 tex_pos;
out vec4 color;
uniform sampler2D sampler;

void main()
{
	color = texture(sampler, tex_pos);
}