#version 460 core

layout (location = 0) in vec3 pos;
uniform mat4 camera;

void main()
{
	gl_Position = camera * vec4(pos, 1.0);
}