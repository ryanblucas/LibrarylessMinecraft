#version 460 core

layout (location = 0) in uint i_pos;
out vec2 tex_pos;
uniform mat4 camera;
uniform mat4 model;

void main()
{
	tex_pos = vec2(((i_pos >> 21) & 255) + ((i_pos >> 19) & 1), (i_pos >> 20) & 1);
	tex_pos.x /= 4.0; // Block count - 1, update w/ adding new blocks

	float strength = float((i_pos >> 29) & 7) + 1;
	strength = 1.0 - ((7.0 - strength) + 1.0) / 8.0F;
	gl_Position = camera * (model * vec4(i_pos & 31, ((i_pos >> 10) & 511) + strength, (i_pos >> 5) & 31, 1.0));
}