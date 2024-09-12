/*
	game.c ~ RL
	Primary game loop
*/

#include "camera.h"
#include "game.h"
#include "graphics.h"
#include "util.h"
#include "window.h"

static shader_t chunk_shader;
static sampler_t atlas;

void game_init(void)
{
	chunk_shader = graphics_shader_load("assets/shaders/basic_vertex.glsl", "assets/shaders/basic_fragment.glsl");
	atlas = graphics_sampler_load("assets/atlas.bmp");

	world_init();
}

void game_destroy(void)
{
	graphics_shader_delete(&chunk_shader);
	graphics_sampler_delete(&atlas);
}

void game_frame(float delta)
{
	world_update(delta);
	current_camera.pos = vector3_add(aabb_get_center(player.hitbox), (vector3_t) { 0.0F, 1.501F - aabb_dimensions(player.hitbox).y / 2.0F, 0.0F });

	graphics_clear(COLOR_CREATE(0x64, 0x95, 0xED));

	graphics_shader_use(chunk_shader);
	graphics_sampler_use(atlas);

	world_render(delta);
}