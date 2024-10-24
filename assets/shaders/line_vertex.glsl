#version 460 core

layout (location = 0) in vec3 pos;
uniform mat4 camera;
uniform mat4 model;

void main()
{
	gl_Position = camera * (model * vec4(pos, 1.0));
}