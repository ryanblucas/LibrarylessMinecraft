/*
	game.c ~ RL
	Primary game loop
*/

#include "camera.h"
#include "game.h"
#include "graphics.h"
#include "interface.h"
#include "util.h"
#include "window.h"

#define TICK_TIME (1.0F / 20)

static shader_t block_shader;
static shader_t liquid_shader;
static sampler_t atlas;

void game_init(void)
{
	block_shader = graphics_shader_load("assets/shaders/block_vertex.glsl", "assets/shaders/block_fragment.glsl");
	liquid_shader = graphics_shader_load("assets/shaders/block_vertex.glsl", "assets/shaders/liquid_fragment.glsl");
	atlas = graphics_sampler_load("assets/atlas.bmp");

	world_init();
	interface_init();
}

void game_destroy(void)
{
	graphics_shader_delete(&block_shader);
	graphics_sampler_delete(&atlas);

	world_destroy();
	interface_destroy();
}

void game_frame(float delta)
{
	static float elapsed = 0.0F;
	elapsed += delta;

	vector3_t curr_pos = aabb_get_center(player.hitbox);
	vector3_t interp_pos = vector3_add(player.prev_position, vector3_mul_scalar(vector3_sub(curr_pos, player.prev_position), elapsed / TICK_TIME));
	camera_update(vector3_add(interp_pos, (vector3_t) { 0.0F, ENTITY_PLAYER_CAMERA_OFFSET - aabb_dimensions(player.hitbox).y / 2.0F, 0.0F }));

	if (window_input_clicked(INPUT_REGENERATE_WORLD))
	{
		world_generate((unsigned int)window_time());
	}

	static bool wireframe_on;
	if (elapsed > TICK_TIME)
	{
		window_input_update();
		player.rotation = (vector3_t){ camera_yaw(), camera_pitch(), 0.0F };
		world_update(elapsed);
		elapsed -= TICK_TIME;

		if (window_input_clicked(INPUT_TOGGLE_WIREFRAME))
		{
			wireframe_on = !wireframe_on;
		}
	}

	graphics_clear(COLOR_CREATE(0x64, 0x95, 0xED));
	graphics_sampler_use(atlas);

	graphics_debug_set_wireframe_mode(wireframe_on);
	world_render(block_shader, liquid_shader, delta);

	graphics_debug_set_wireframe_mode(false);
	interface_frame();
}