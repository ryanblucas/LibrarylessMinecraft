/*
	world.c ~ RL
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>
#include "entity.h"
#include "graphics.h"

#define ROUND_DOWN(c, m) (((c) < 0 ? -((-(c) - 1 + (m)) / (m)) : (c) / (m)) * (m))
#define BUCKET_COUNT 32

array_list_t chunk_list;
static array_list_t update_list;
static int ticks;

entity_t player;

struct chunk* world_chunk_create(int x_o, int z_o)
{
	int res = mc_list_add(chunk_list, mc_list_count(chunk_list), NULL, sizeof(struct chunk));
	struct chunk* next = MC_LIST_CAST_GET(chunk_list, res, struct chunk);

	next->x = ROUND_DOWN(x_o, CHUNK_WX);
	next->z = ROUND_DOWN(z_o, CHUNK_WZ);
	next->flowing_liquid = mc_list_create(sizeof(struct liquid));

	for (int i = 0; i < CHUNK_FLOOR_BLOCK_COUNT; i++)
	{
		int x = CHUNK_X(i), z = CHUNK_Z(i);
		for (int j = CHUNK_WY - 1; j >= 129; j--)
		{
			next->arr[CHUNK_INDEX_OF(x, j, z)] = BLOCK_AIR;
		}
		next->arr[CHUNK_INDEX_OF(x, 128, z)] = BLOCK_GRASS;/*
		for (int j = 127; j >= 125; j--)
		{
			next->arr[CHUNK_INDEX_OF(x, j, z)] = BLOCK_DIRT;
		}
		for (int j = 124; j >= 0; j--)
		{
			next->arr[CHUNK_INDEX_OF(x, j, z)] = BLOCK_STONE;
		}*/
	}
	next->dirty_mask = OPAQUE_BIT;
	next->opaque_buffer = graphics_buffer_create(NULL, 0);
	next->liquid_buffer = graphics_buffer_create(NULL, 0);
	return next;
}

struct chunk* world_chunk_get(int x, int z)
{
	x = ROUND_DOWN(x, CHUNK_WX);
	z = ROUND_DOWN(z, CHUNK_WZ);
	struct chunk* list = mc_list_array(chunk_list);
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		if (list[i].x == x && list[i].z == z)
		{
			return &list[i];
		}
	}
	return NULL;
}

void world_init(void)
{
	chunk_list = mc_list_create(sizeof(struct chunk));
	update_list = mc_list_create(sizeof(block_coords_t));

	world_chunk_create(0, 0);
	entity_player_init(&player);
}

void world_destroy(void)
{
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->opaque_buffer);
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->liquid_buffer);
	}
	mc_list_destroy(chunk_list);
	mc_list_destroy(&update_list);
}

struct ray_state
{
	ray_t curr;
	float len;
	vector3_t inv_dir, dir, start;
};

void world_raycast_loop(block_coords_t i, void* user)
{
	if (!IS_SOLID(world_block_get(i)))
	{
		return;
	}

	struct ray_state* state = (struct ray_state*)user;
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

ray_t world_ray_cast(vector3_t start, vector3_t direction, float len)
{
	direction = vector3_normalize(direction);
	struct ray_state state =
	{
		.curr = {.min = start, .max = vector3_add(start, vector3_mul_scalar(direction, len)), .block = {.y = -1}},
		.dir = direction,
		.inv_dir = vector3_scalar_div(1.0F, direction),
		.start = start,
		.len = len
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

int world_region_aabb(block_coords_t _min, block_coords_t _max, aabb_t* arr, size_t arr_len)
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

static void world_block_update(block_coords_t coords)
{
	if (IS_INVALID_BLOCK_COORDS(coords) || !world_chunk_get(coords.x, coords.z))
	{
		return;
	}

	graphics_debug_set_cube(block_coords_to_vector(coords), (vector3_t) { 1, 1, 1 });
	mc_list_add(update_list, mc_list_count(update_list), &coords, sizeof coords);
}

static struct liquid* world_liquid_get(block_coords_t coords)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk)
	{
		return NULL;
	}
	for (int i = 0; i < mc_list_count(chunk->flowing_liquid); i++)
	{
		struct liquid* curr = MC_LIST_CAST_GET(chunk->flowing_liquid, i, struct liquid);
		if (is_block_coords_equal(coords, curr->position))
		{
			return curr;
		}
	}
	return NULL;
}

static void world_liquid_add(block_coords_t coords, block_coords_t origin, int strength)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	struct liquid* existing = world_liquid_get(coords);
	if (!chunk || strength <= 0 || IS_SOLID(world_block_get(coords)) || existing)
	{
		return;
	}
	struct liquid to_add =
	{
		.origin = origin,
		.position = coords,
		.strength = strength,
	};
	mc_list_add(chunk->flowing_liquid, mc_list_count(chunk->flowing_liquid), &to_add, sizeof to_add);
	world_block_update(coords);
	chunk->dirty_mask |= LIQUID_BIT;
}

static void world_block_tick(void)
{
	if (ticks % 20 != 0)
	{
		return;
	}

	int old_count = mc_list_count(update_list);
	for (int i = old_count - 1; i >= 0; i--)
	{
		block_coords_t coords = *MC_LIST_CAST_GET(update_list, i, block_coords_t);
		bool is_source = world_block_get(coords) == BLOCK_WATER;
		struct liquid* pflow = world_liquid_get(coords);
		if (!is_source && !pflow)
		{
			continue;
		}
		else if (is_source && !pflow)
		{
			world_liquid_add(coords, coords, 8);
			continue;
		}

		world_liquid_add((block_coords_t) { coords.x - 1, coords.y, coords.z }, pflow->origin, pflow->strength - 1);
		world_liquid_add((block_coords_t) { coords.x + 1, coords.y, coords.z }, pflow->origin, pflow->strength - 1);
		world_liquid_add((block_coords_t) { coords.x, coords.y - 1, coords.z }, pflow->origin, pflow->strength - 1);
		world_liquid_add((block_coords_t) { coords.x, coords.y, coords.z - 1 }, pflow->origin, pflow->strength - 1);
		world_liquid_add((block_coords_t) { coords.x, coords.y, coords.z + 1 }, pflow->origin, pflow->strength - 1);
	}
	mc_list_splice(update_list, 0, old_count);
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
	world_block_update((block_coords_t) { coords.x - 1, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x + 1, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y - 1, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y + 1, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z - 1 });
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z + 1 });
}

void world_update(float delta)
{
	world_block_tick();
	entity_player_update(&player, delta);
	ticks++;
}

void world_render(float delta)
{
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		struct chunk* chunk = MC_LIST_CAST_GET(chunk_list, i, struct chunk);
		world_chunk_clean_mesh(i);

		matrix_t transform;
		matrix_translation((vector3_t) { (float)chunk->x, 0.0F, (float)chunk->z }, transform);
		graphics_shader_matrix("model", transform);

		graphics_buffer_draw(chunk->opaque_buffer);
		graphics_buffer_draw(chunk->liquid_buffer);
	}
}