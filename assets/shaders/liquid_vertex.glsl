#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 i_tex_pos;
uniform mat4 camera;
uniform mat4 model;
out vec2 tex_pos;

void main()
{
	tex_pos = i_tex_pos;
	gl_Position = camera * (model * vec4(pos, 1.0));
}