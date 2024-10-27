#version 460 core

flat in int colori;
out vec4 color;

void main()
{
	color = vec4((colori & 0xFF) / 255.0F, ((colori >> 8) & 0xFF) / 255.0F, ((colori >> 16) & 0xFF) / 255.0F, 1.0);
}