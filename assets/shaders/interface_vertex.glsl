#version 460 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 i_tex_pos;
layout (location = 2) in int i_tint;

uniform int width;
uniform int height;

out vec2 tex_pos;
flat out int tint;

void main()
{
	tex_pos = i_tex_pos;
	tint = i_tint;

	vec3 _pos = pos;
	_pos.x /= width / 2;
	_pos.y /= height / 2;
	_pos.xy -= 1.0;

	tex_pos.y = 1.0 - tex_pos.y;
	_pos.y = -_pos.y;

	gl_Position = vec4(_pos, 1.0);
}