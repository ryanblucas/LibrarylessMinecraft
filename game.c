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

static debug_buffer_t current_block;

void game_init(void)
{
	block_shader = graphics_shader_load("assets/shaders/block_vertex.glsl", "assets/shaders/block_fragment.glsl");
	liquid_shader = graphics_shader_load("assets/shaders/block_vertex.glsl", "assets/shaders/liquid_fragment.glsl");
	atlas = graphics_sampler_load("assets/atlas.bmp");

	world_init();
	interface_init(atlas);

	float verts[72];
	graphics_primitive_cube((vector3_t) { 0 }, (vector3_t) { 1, 1, 1 }, verts);
	current_block.vertex = graphics_buffer_create(verts, sizeof verts / sizeof * verts / 3, VERTEX_POSITION);
}

void game_destroy(void)
{
	graphics_shader_delete(&block_shader);
	graphics_sampler_delete(&atlas);

	world_destroy();
	interface_destroy();
	graphics_buffer_delete(&current_block.vertex);
}

void game_frame(float delta)
{
	static float elapsed = 0.0F;
	elapsed += delta;

	vector3_t curr_pos = aabb_get_center(player.hitbox);
	vector3_t interp_pos = vector3_add(player.prev_position, vector3_mul_scalar(vector3_sub(curr_pos, player.prev_position), elapsed / TICK_TIME));
	vector3_t cam_pos = vector3_add(interp_pos, (vector3_t) { 0.0F, ENTITY_PLAYER_CAMERA_OFFSET - aabb_get_dimensions(player.hitbox).y / 2.0F, 0.0F });
	camera_update(cam_pos);

	if (window_input_clicked(INPUT_REGENERATE_WORLD))
	{
		world_generate((unsigned int)window_time());
	}

	static bool wireframe_on;
	if (elapsed > TICK_TIME)
	{
		interface_set_underwater_state(false);
		if (world_block_get(vector_to_block_coords(cam_pos)) == BLOCK_WATER)
		{
			interface_set_underwater_state(true);
		}

		window_input_update();
		player.rotation = (vector3_t){ camera_yaw(), camera_pitch(), 0.0F };
		world_update(elapsed);
		interface_update();
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
	interface_render();

	ray_t ray = world_ray_cast(camera_position(), camera_forward(), 5.0F, RAY_SOLID);
	if (!IS_INVALID_BLOCK_COORDS(ray.block))
	{
		current_block.position = (vector3_t){ ray.block.x, ray.block.y, ray.block.z };
		graphics_debug_queue_buffer(current_block);
	}
}