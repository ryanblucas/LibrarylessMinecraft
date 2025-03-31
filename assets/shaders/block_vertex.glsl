#version 460 core

layout (location = 0) in uint i_pos;
out vec2 tex_pos;
uniform mat4 camera;
uniform mat4 model;

void main()
{
	tex_pos = vec2(((i_pos >> 21) & 255) + ((i_pos >> 19) & 1), (5.0 - ((i_pos >> 29) & 7)) + ((i_pos >> 20) & 1));
	tex_pos.x /= 10.0; // Block count - 1, update w/ adding new blocks
	tex_pos.y /= 6.0;
	gl_Position = camera * (model * vec4(i_pos & 31, ((i_pos >> 10) & 511), (i_pos >> 5) & 31, 1.0));
}