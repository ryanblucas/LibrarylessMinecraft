#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 i_tex_pos;
out vec2 tex_pos;
uniform mat4 camera;

void main()
{
	tex_pos = i_tex_pos;
	gl_Position = camera * vec4(pos, 1.0);
}