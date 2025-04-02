#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 i_tex_pos;

uniform int width;
uniform int height;

out vec2 tex_pos;

void main()
{
	tex_pos = i_tex_pos;

	vec3 _pos = pos;
	_pos.x /= width / 2;
	_pos.y /= height / 2;
	_pos.xy -= 1.0;
	gl_Position = vec4(_pos, 1.0);
}