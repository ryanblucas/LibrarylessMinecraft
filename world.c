/*
	world.c ~ RL
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>
#include "entity.h"
#include "graphics.h"
#include "window.h"

static array_list_t update_list;
static int ticks;

static vertex_buffer_t debug_chunk_border;
static bool display_debug_chunk_border;

entity_t player;

void world_init(void)
{
	update_list = mc_list_create(sizeof(block_coords_t));
	world_chunk_init();
	entity_player_init(&player);

	array_list_t vertices = mc_list_create(sizeof(vertex_t));
	for (int j = 0; j < CHUNK_WY; j += 16)
	{
		vertex_t to_add[24];
		graphics_primitive_cube((vector3_t) { 0, (float)j, 0 }, (vector3_t) { 16, 16, 16 }, to_add);
		mc_list_array_add(vertices, mc_list_count(vertices), to_add, sizeof * to_add, 24);
	}
	debug_chunk_border = graphics_buffer_create(mc_list_array(vertices), mc_list_count(vertices), STANDARD_VERTEX);
	mc_list_destroy(&vertices);
}

void world_destroy(void)
{
	world_chunk_destroy();
	mc_list_destroy(&update_list);
	graphics_buffer_delete(&debug_chunk_border);
}

struct ray_state
{
	ray_t curr;
	float len;
	vector3_t inv_dir, dir, start;
	ray_settings_t settings;
};

void world_raycast_loop(block_coords_t i, void* user)
{
	struct ray_state* state = (struct ray_state*)user;
	block_type_t type = world_block_get(i);
	if (!(state->settings & RAY_SOLID && IS_SOLID(type))
		&& !(state->settings & RAY_AIR && type == BLOCK_AIR)
		&& !(state->settings & RAY_LIQUID && world_liquid_get(i)))
	{
		return;
	}

	vector3_t box_min = block_coords_to_vector(i),
		box_max = vector3_add_scalar(box_min, 1.0F);

	vector3_t t0 = vector3_mul(vector3_sub(box_min, state->start), state->inv_dir),
		t1 = vector3_mul(vector3_sub(box_max, state->start), state->inv_dir);
	vector3_t t_min = vector3_min(t0, t1),
		t_max = vector3_max(t0, t1);

	float t_pmin = max(t_min.x, t_min.y),
		t_pmax = min(t_max.x, t_max.y);
	t_pmin = max(t_pmin, t_min.z);
	t_pmax = min(t_pmax, t_max.z);

	if (t_pmax >= t_pmin && t_pmax >= 0)
	{
		float this_dist = vector3_distance(vector3_add_scalar(box_min, 0.5F), state->start);
		if (this_dist > state->len)
		{
			return;
		}
		state->curr.block = i;
		state->curr.min = vector3_add(state->start, vector3_mul_scalar(state->dir, t_pmin));
		state->curr.max = vector3_add(state->start, vector3_mul_scalar(state->dir, t_pmax));
		state->len = this_dist;
	}
}

ray_t world_ray_cast(vector3_t start, vector3_t direction, float len, ray_settings_t settings)
{
	direction = vector3_normalize(direction);
	struct ray_state state =
	{
		.curr = {.min = start, .max = vector3_add(start, vector3_mul_scalar(direction, len)), .block = {.y = -1}},
		.dir = direction,
		.inv_dir = vector3_scalar_div(1.0F, direction),
		.start = start,
		.len = len,
		.settings = settings
	};
	world_region_loop(vector_to_block_coords(start), vector_to_block_coords(state.curr.max), world_raycast_loop, &state);
	return state.curr;
}

block_coords_t world_ray_neighbor(ray_t ray)
{
	block_coords_t res = ray.block;
	if (IS_INVALID_BLOCK_COORDS(res))
	{
		return res;
	}

	vector3_t is_centered = vector3_sub(ray.min, vector3_add_scalar(block_coords_to_vector(ray.block), 0.5F)),
		is_c_abs = vector3_abs(is_centered);
	if (is_c_abs.x > is_c_abs.y && is_c_abs.x > is_c_abs.z)
	{
		res.x += is_centered.x < 0.0F ? -1 : 1;
	}
	else if (is_c_abs.y > is_c_abs.x && is_c_abs.y > is_c_abs.z)
	{
		res.y += is_centered.y < 0.0F ? -1 : 1;
	}
	else
	{
		res.z += is_centered.z < 0.0F ? -1 : 1;
	}

	return res;
}

void world_region_loop(block_coords_t _min, block_coords_t _max, world_loop_callback_t callback, void* user)
{
	vector3_t v_min = block_coords_to_vector(_min),
		v_max = block_coords_to_vector(_max);
	_min = vector_to_block_coords(vector3_min(v_min, v_max));
	_max = vector_to_block_coords(vector3_max(v_min, v_max));

	block_coords_t curr;
	for (curr.z = _min.z; curr.z <= _max.z; curr.z++)
	{
		for (curr.y = _min.y; curr.y <= _max.y; curr.y++)
		{
			for (curr.x = _min.x; curr.x <= _max.x; curr.x++)
			{
				callback(curr, user);
			}
		}
	}
}

int world_region_aabb(block_coords_t _min, block_coords_t _max, aabb_t* arr, int arr_len)
{
	vector3_t v_min = block_coords_to_vector(_min),
		v_max = block_coords_to_vector(_max);
	_min = vector_to_block_coords(vector3_min(v_min, v_max));
	_max = vector_to_block_coords(vector3_max(v_min, v_max));

	block_coords_t curr;
	int curr_i = 0;
	for (curr.z = _min.z; curr.z <= _max.z; curr.z++)
	{
		for (curr.y = _min.y; curr.y <= _max.y; curr.y++)
		{
			for (curr.x = _min.x; curr.x <= _max.x; curr.x++)
			{
				if (curr_i >= arr_len || !IS_SOLID(world_block_get(curr)))
				{
					continue;
				}
				aabb_t aabb = { .min = block_coords_to_vector(curr) };
				aabb.max = vector3_add_scalar(aabb.min, 1.0F);
				arr[curr_i++] = aabb;
			}
		}
	}
	return curr_i;
}

aabb_t world_liquid_to_aabb(liquid_t liquid)
{
	float s = 1 - ((7 - liquid.strength) + 1) / 8.0F;
	vector3_t min = block_coords_to_vector(liquid.position);
	return (aabb_t) { min, vector3_add(min, (vector3_t) { 1.0F, s, 1.0F }) };
}

static int update_list_start = 0;

void world_block_update(block_coords_t coords)
{
	if (IS_INVALID_BLOCK_COORDS(coords) || !world_chunk_get(coords.x, coords.z))
	{
		return;
	}

	for (int i = update_list_start; i < mc_list_count(update_list); i++)
	{
		if (is_block_coords_equal(*MC_LIST_CAST_GET(update_list, i, block_coords_t), coords))
		{
			return;
		}
	}

	mc_list_add(update_list, mc_list_count(update_list), &coords, sizeof coords);
}

static liquid_t* world_liquid_get(block_coords_t coords)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk)
	{
		return NULL;
	}
	for (int i = 0; i < mc_list_count(chunk->flowing_liquid); i++)
	{
		liquid_t* curr = MC_LIST_CAST_GET(chunk->flowing_liquid, i, liquid_t);
		if (is_block_coords_equal(coords, curr->position))
		{
			return curr;
		}
	}
	return NULL;
}

static void world_liquid_remove(block_coords_t coords)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk)
	{
		return;
	}
	for (int i = mc_list_count(chunk->flowing_liquid) - 1; i >= 0; i--)
	{
		if (is_block_coords_equal(coords, MC_LIST_CAST_GET(chunk->flowing_liquid, i, liquid_t)->position))
		{
			mc_list_remove(chunk->flowing_liquid, i, NULL, 0);
			world_block_update(coords);
			chunk->dirty_mask |= LIQUID_BIT;
		}
	}
}

static liquid_t* world_liquid_add(block_coords_t coords, vector3_t push, int strength)
{
	world_liquid_remove(coords);
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk || IS_SOLID(world_block_get(coords)))
	{
		return NULL;
	}
	liquid_t to_add = { .position = coords, .push = push, .strength = strength, .tick_count = ticks };
	to_add.came_from_above = !!world_liquid_get((block_coords_t) { coords.x, coords.y + 1, coords.z });

	int i = mc_list_add(chunk->flowing_liquid, mc_list_count(chunk->flowing_liquid), &to_add, sizeof to_add);
	chunk->dirty_mask |= LIQUID_BIT;
	world_block_update(coords);
	return MC_LIST_CAST_GET(chunk->flowing_liquid, i, liquid_t);
}

static inline int world_liquid_strength(block_coords_t coords)
{
	liquid_t* liquid = world_liquid_get(coords);
	if (!liquid)
	{
		return 0;
	}
	return liquid->strength;
}

static void world_liquid_spread(block_coords_t coords)
{
	int l_s = world_liquid_strength((block_coords_t) { coords.x - 1, coords.y, coords.z }),
		r_s = world_liquid_strength((block_coords_t) { coords.x + 1, coords.y, coords.z }),
		f_s = world_liquid_strength((block_coords_t) { coords.x, coords.y, coords.z - 1 }),
		b_s = world_liquid_strength((block_coords_t) { coords.x, coords.y, coords.z + 1 });
	int strength = max(l_s, max(r_s, max(f_s, b_s))) - 1;

	liquid_t* existing = world_liquid_get(coords);
	if (existing && existing->strength == WATER_STRENGTH)
	{
		return;
	}

	if (strength <= 0)
	{
		world_liquid_remove(coords);
		return;
	}

	vector3_t l_p = vector3_mul_scalar((vector3_t) { -1.0F, 0.0F, 0.0F }, 8.0F - (float)l_s),
		r_p = vector3_mul_scalar((vector3_t) { 1.0F, 0.0F, 0.0F }, 8.0F - (float)r_s),
		f_p = vector3_mul_scalar((vector3_t) { 0.0F, 0.0F, -1.0F }, 8.0F - (float)f_s),
		b_p = vector3_mul_scalar((vector3_t) { 0.0F, 0.0F, 1.0F }, 8.0F - (float)b_s);
	vector3_t push = vector3_normalize(vector3_add(vector3_add(l_p, r_p), vector3_add(f_p, b_p)));

	/* TO DO: would be simpler if you just called world_liquid_add, since the effect (should) be the same, but a simple replacement doesn't work quite right. */

	if (!existing)
	{
		world_liquid_add(coords, push, strength);
		return;
	}

	existing->push = push;
	if (existing->strength != strength)
	{
		existing->strength = strength;
		existing->tick_count = ticks;

		world_block_update(coords);
		world_chunk_get(coords.x, coords.z)->dirty_mask |= LIQUID_BIT;
	}
}

static void world_liquid_try_tick(block_coords_t coords)
{
	bool is_source = world_block_get(coords) == BLOCK_WATER;
	liquid_t* pflow = world_liquid_get(coords);
	if (!is_source && !pflow)
	{
		return;
	}
	else if (is_source && !pflow)
	{
		world_liquid_add(coords, (vector3_t) { 0 }, WATER_STRENGTH);
		return;
	}

	block_coords_t down = (block_coords_t){ coords.x, coords.y - 1, coords.z };
	if (IS_SOLID(world_block_get(coords)) || (pflow->came_from_above && !world_liquid_get((block_coords_t) { coords.x, coords.y + 1, coords.z })))
	{
		world_liquid_remove(coords);
	}

	if (ticks - pflow->tick_count < 5)
	{
		world_block_update(coords);
		return;
	}

	world_liquid_spread(coords);
	if (IS_SOLID(world_block_get(down)))
	{
		world_liquid_spread((block_coords_t) { coords.x - 1, coords.y, coords.z });
		world_liquid_spread((block_coords_t) { coords.x + 1, coords.y, coords.z });
		world_liquid_spread((block_coords_t) { coords.x, coords.y, coords.z - 1 });
		world_liquid_spread((block_coords_t) { coords.x, coords.y, coords.z + 1 });
		return;
	}
	if (!world_liquid_get(down))
	{
		world_liquid_add(down, (vector3_t) { 0 }, WATER_STRENGTH);
	}
}

static void world_block_tick(void)
{
	/* TEMPORARY FIX FOR WATER *TO DO* -- this is slow and makes water clean in a random way */
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		array_list_t water = MC_LIST_CAST_GET(chunk_list, i, struct chunk)->flowing_liquid;
		for (int j = 0; j < mc_list_count(water); j++)
		{
			world_block_update(MC_LIST_CAST_GET(water, j, liquid_t)->position);
		}
	}

	update_list_start = mc_list_count(update_list);
	for (int i = update_list_start - 1; i >= 0; i--)
	{
		block_coords_t coords = *MC_LIST_CAST_GET(update_list, i, block_coords_t);
		world_liquid_try_tick(coords);
	}
	mc_list_splice(update_list, 0, update_list_start);
	update_list_start = 0;
}

block_type_t world_block_get(block_coords_t coords)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (IS_INVALID_BLOCK_COORDS(coords) || !chunk)
	{
		return BLOCK_AIR;
	}

	coords.x -= chunk->x;
	coords.z -= chunk->z;
	return CHUNK_AT(chunk->arr, coords.x, coords.y, coords.z);
}

void world_block_set(block_coords_t coords, block_type_t type)
{
	if (IS_INVALID_BLOCK_COORDS(coords))
	{
		return;
	}

	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk)
	{
		chunk = world_chunk_create(coords.x, coords.z);
	}

	chunk->arr[CHUNK_INDEX_OF(coords.x - chunk->x, coords.y, coords.z - chunk->z)] = type;
	chunk->dirty_mask |= OPAQUE_BIT;
	
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z });
}

void world_block_debug(block_coords_t coords, FILE* stream)
{
	block_type_t id = world_block_get(coords);

	const char* name = "UNKNOWN";
#define DEFINE_BLOCK(block) case BLOCK_ ## block: name = #block; break;
	switch (id)
	{
		BLOCK_LIST
	}
#undef DEFINE_BLOCK

	vector3_t push = { 0 };
	liquid_t* liquid = world_liquid_get(coords);
	if (liquid)
	{
		push = liquid->push;
	}

	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	int i;
	for (i = 0; i < mc_list_count(chunk_list); i++)
	{
		if (chunk == MC_LIST_CAST_GET(chunk_list, i, struct chunk))
		{
			break;
		}
	}

	fprintf(stream, "(%i, %i, %i), chunk %i (%i, %i). Block name \"%s,\" block id: %i. Liquid strength: %i, liquid push: (%f, %f, %f)\n", 
		coords.x, coords.y, coords.z, i, chunk->x, chunk->z, name, id, world_liquid_strength(coords), push.x, push.y, push.z);
}

void world_update(float delta)
{
	world_block_tick();
	entity_player_update(&player, delta);

	struct chunk* player_chunk = world_chunk_get((int)aabb_get_center(player.hitbox).x, (int)aabb_get_center(player.hitbox).z);
	if (player_chunk)
	{
		for (int i = 0; i < mc_list_count(player_chunk->flowing_liquid); i++)
		{
			liquid_t* curr = MC_LIST_CAST_GET(player_chunk->flowing_liquid, i, liquid_t);
			if (aabb_collides_aabb(world_liquid_to_aabb(*curr), player.hitbox))
			{
				entity_move(&player, vector3_mul_scalar(curr->push, delta));
			}
		}
	}

	if (window_input_clicked(INPUT_TOGGLE_CHUNK_BORDERS))
	{
		display_debug_chunk_border = !display_debug_chunk_border;
	}

	ticks++;
}

void world_render(float delta)
{
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		struct chunk* chunk = MC_LIST_CAST_GET(chunk_list, i, struct chunk);
		world_chunk_clean_mesh(i);

		/* Two transforms being generated is temporary. When a real water shader is made, this isn't needed */
		matrix_t transform;
		matrix_translation((vector3_t) { (float)chunk->x, -0.125F, (float)chunk->z }, transform);
		graphics_shader_matrix("model", transform);
		graphics_buffer_draw(chunk->opaque_buffer);

		matrix_translation((vector3_t) { (float)chunk->x, 0.0F, (float)chunk->z }, transform);
		graphics_shader_matrix("model", transform);
		graphics_buffer_draw(chunk->liquid_buffer);
	}

	if (display_debug_chunk_border)
	{
		vector3_t player_pos = aabb_get_center(player.hitbox);
		player_pos.x = ROUND_DOWN(player_pos.x, CHUNK_WX);
		player_pos.y = 0.0F;
		player_pos.z = ROUND_DOWN(player_pos.z, CHUNK_WZ);
		graphics_debug_queue_buffer((debug_buffer_t)
		{
			.vertex = debug_chunk_border,
			.color = COLOR_CREATE(255, 0, 0),
			.position = player_pos
		});
	}
}