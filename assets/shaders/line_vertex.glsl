#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in int i_color;
uniform mat4 camera;
uniform mat4 model;
flat out int colori;

void main()
{
	colori = i_color;
	gl_Position = camera * (model * vec4(pos, 1.0));
}