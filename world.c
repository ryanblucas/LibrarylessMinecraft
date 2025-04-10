/*
	world.c ~ RL
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>
#include "camera.h"
#include "entity.h"
#include "graphics.h"
#include "window.h"

static hash_set_t update_list, old_update_list;
static int ticks;

entity_t player;

void world_init(void)
{
	update_list = mc_set_create(sizeof(block_coords_t));
	old_update_list = mc_set_create(sizeof(block_coords_t));
	world_chunk_init((unsigned int)window_time());
	world_render_init();

	entity_player_init(&player);
}

void world_destroy(void)
{
	world_chunk_destroy();
	world_render_destroy();
	mc_set_destroy(&update_list);
	mc_set_destroy(&old_update_list);
}

void world_generate(unsigned int seed)
{
	world_chunk_destroy();
	world_chunk_init(seed);
}

int world_ticks(void)
{
	return ticks;
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
		&& !(state->settings & RAY_LIQUID && type == BLOCK_WATER))
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
	direction = vector3_mul_scalar(direction, 10.0F); /* Band-aid? its really just a rounding issue TO DO */
	world_region_loop(vector_to_block_coords(vector3_sub(start, direction)), vector_to_block_coords(vector3_add(state.curr.max, direction)), world_raycast_loop, &state);
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

void world_block_update(block_coords_t coords)
{
	if (IS_INVALID_BLOCK_COORDS(coords) || !world_chunk_get(coords.x, coords.z))
	{
		return;
	}

	mc_set_add(update_list, &coords, sizeof coords);
}

const char* world_block_stringify(block_type_t type)
{
	const char* name = "UNKNOWN";
#define DEFINE_BLOCK(block) case BLOCK_ ## block: name = #block; break;
	switch (type)
	{
		BLOCK_LIST
	}
#undef DEFINE_BLOCK
	return name;
}

static inline void world_block_try_set(block_coords_t coords, block_type_t type)
{
	if (world_block_get(coords) == BLOCK_AIR)
	{
		world_block_set(coords, type);
	}
}

static bool world_block_tick_callback(const hash_set_t set, void* value, void* user)
{
	block_coords_t coords = *(block_coords_t*)value;
	if (world_block_get(coords) == BLOCK_WATER)
	{
		world_block_try_set((block_coords_t) { coords.x + 1, coords.y, coords.z }, BLOCK_WATER);
		world_block_try_set((block_coords_t) { coords.x - 1, coords.y, coords.z }, BLOCK_WATER);
		world_block_try_set((block_coords_t) { coords.x, coords.y, coords.z + 1 }, BLOCK_WATER);
		world_block_try_set((block_coords_t) { coords.x, coords.y, coords.z - 1 }, BLOCK_WATER);
		world_block_try_set((block_coords_t) { coords.x, coords.y - 1, coords.z }, BLOCK_WATER);
	}
	return true;
}

static void world_block_tick(void)
{
	if (ticks % 5 != 0) /* only water is updated RN */
	{
		return;
	}

	hash_set_t temp = old_update_list;
	old_update_list = update_list;
	update_list = temp;
	mc_set_iterate(old_update_list, world_block_tick_callback, NULL);
	mc_set_clear(old_update_list);
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

	world_block_update((block_coords_t) { coords.x, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x - 1, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x + 1, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y - 1, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y + 1, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z - 1 });
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z + 1 });

	chunk->arr[CHUNK_INDEX_OF(coords.x - chunk->x, coords.y, coords.z - chunk->z)] = type;
	int bit = type == BLOCK_WATER ? LIQUID_BIT : OPAQUE_BIT;
	chunk->dirty_mask |= bit;

	struct chunk* neighbor = NULL;
	if (coords.x - chunk->x == 0)
	{
		neighbor = world_chunk_get(coords.x - 1, coords.z);
	}
	else if (coords.x - chunk->x == 15)
	{
		neighbor = world_chunk_get(coords.x + 1, coords.z);
	}
	assert(neighbor != chunk);
	if (neighbor)
	{
		neighbor->dirty_mask |= bit;
	}
	if (coords.z - chunk->z == 0)
	{
		neighbor = world_chunk_get(coords.x, coords.z - 1);
	}
	else if (coords.z - chunk->z == 15)
	{
		neighbor = world_chunk_get(coords.x, coords.z + 1);
	}
	assert(neighbor != chunk);
	if (neighbor)
	{
		neighbor->dirty_mask |= bit;
	}
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

	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	int i;
	for (i = 0; i < mc_list_count(chunk_list); i++)
	{
		if (chunk == MC_LIST_CAST_GET(chunk_list, i, struct chunk))
		{
			break;
		}
	}

	fprintf(stream, "(%i, %i, %i), chunk %i (%i, %i). Block name \"%s,\" block id: %i\n", 
		coords.x, coords.y, coords.z, i, chunk->x, chunk->z, name, id);
}

void world_update(float delta)
{
	if (window_input_clicked(INPUT_QUEUE_BLOCK_INFO))
	{
		block_coords_t bc = world_ray_cast(camera_position(), camera_forward(), 16.0F, RAY_SOLID | RAY_LIQUID).block;
		GRAPHICS_DEBUG_SET_BLOCK(bc);
		world_block_debug(bc, stderr);
	}
	if (window_input_clicked(INPUT_UPDATE_BLOCK))
	{
		block_coords_t bc = world_ray_cast(camera_position(), camera_forward(), 16.0F, RAY_SOLID | RAY_LIQUID).block;
		GRAPHICS_DEBUG_SET_BLOCK(bc);
		world_block_update(bc);
	}

	world_block_tick();
	entity_player_update(&player, delta);
	world_chunk_update();

	ticks++;
}