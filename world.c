/*
	world.c ~ RL
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>
#include "entity.h"
#include "graphics.h"

#define ROUND_DOWN(c, m) (((c) < 0 ? -((-(c) - 1 + (m)) / (m)) : (c) / (m)) * (m))

struct chunk
{
	int x, z; /* The x and z coordinates in block space. As in, these numbers are multiples of 16 (chunk width and depth.) */
	bool is_dirty;
	vertex_buffer_t opaque_buffer;

	/* Since most of this is air anyways, this could set a variable height to save memory, TO DO */
	block_type_t arr[CHUNK_BLOCK_COUNT];
};

static struct chunk_list
{
	int reserved, count;
	struct chunk* arr;
} chunk_list;

entity_t player;

static inline struct chunk* world_chunk_get(int x, int z)
{
	x = ROUND_DOWN(x, CHUNK_WX);
	z = ROUND_DOWN(z, CHUNK_WZ);
	for (int i = 0; i < chunk_list.count; i++)
	{
		if (chunk_list.arr[i].x == x && chunk_list.arr[i].z == z)
		{
			return &chunk_list.arr[i];
		}
	}
	return NULL;
}

static struct chunk* world_chunk_create(int x_o, int z_o)
{
	struct chunk* next = world_chunk_get(x_o, z_o);
	if (world_chunk_get(x_o, z_o) != NULL)
	{
		return next;
	}

	int next_pos = chunk_list.count++;
	if (chunk_list.count >= chunk_list.reserved)
	{
		int new_reserved = chunk_list.reserved * 2;
		struct chunk* new_arr = mc_malloc(sizeof * chunk_list.arr * new_reserved);
		memcpy(new_arr, chunk_list.arr, sizeof * chunk_list.arr * chunk_list.reserved);
		free(chunk_list.arr);
		chunk_list.arr = new_arr;
		chunk_list.reserved = new_reserved;
	}
	next = &chunk_list.arr[next_pos];
	next->x = ROUND_DOWN(x_o, CHUNK_WX);
	next->z = ROUND_DOWN(z_o, CHUNK_WZ);
	for (int i = 0; i < CHUNK_FLOOR_BLOCK_COUNT; i++)
	{
		int x = CHUNK_X(i), z = CHUNK_Z(i);
		for (int j = CHUNK_WY - 1; j >= 129; j--)
		{
			world_block_set((block_coords_t) { next->x + x, j, next->z + z }, BLOCK_AIR);
		}
		world_block_set((block_coords_t) { next->x + x, 128, next->z + z }, BLOCK_GRASS);
		for (int j = 127; j >= 125; j--)
		{
			world_block_set((block_coords_t) { next->x + x, j, next->z + z }, BLOCK_DIRT);
		}
		for (int j = 124; j >= 0; j--)
		{
			world_block_set((block_coords_t) { next->x + x, j, next->z + z }, BLOCK_STONE);
		}
	}
	next->opaque_buffer = graphics_buffer_create(NULL, 0);
	return next;
}

void world_init(void)
{
	entity_player_init(&player);

	chunk_list.reserved = 32 * 32;
	chunk_list.arr = mc_malloc(sizeof * chunk_list.arr * chunk_list.reserved);
	world_chunk_create(0, 0);
}

void world_destroy(void)
{
	for (int i = 0; i < chunk_list.count; i++)
	{
		graphics_buffer_delete(&chunk_list.arr[i].opaque_buffer);
	}
	free(chunk_list.arr);
	chunk_list.arr = NULL;
	chunk_list.reserved = 0;
}

struct ray_state
{
	ray_t curr;
	float len;
	vector3_t inv_dir, dir, start;
};

void world_raycast_loop(block_coords_t i, void* user)
{
	if (world_block_get(i) == BLOCK_AIR)
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
				if (curr_i >= arr_len || world_block_get(curr) == BLOCK_AIR)
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

block_type_t world_block_get(block_coords_t coords)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (IS_INVALID_BLOCK_COORDS(coords) || !chunk)
	{
		return BLOCK_AIR;
	}

	coords.x -= chunk->x;
	coords.z -= chunk->z;
	return chunk->arr[CHUNK_INDEX_OF(coords.x, coords.y, coords.z)];
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

	coords.x -= chunk->x;
	coords.z -= chunk->z;
	chunk->arr[CHUNK_INDEX_OF(coords.x, coords.y, coords.z)] = type;
	chunk->is_dirty = true;
}

void world_update(float delta)
{
	entity_player_update(&player, delta);
}

void world_render(float delta)
{
	for (int i = 0; i < chunk_list.count; i++)
	{
		if (chunk_list.arr[i].is_dirty)
		{
			world_chunk_mesh(chunk_list.arr[i].opaque_buffer, chunk_list.arr[i].arr);
			chunk_list.arr[i].is_dirty = false;
		}

		matrix_t transform;
		matrix_translation((vector3_t) { (float)chunk_list.arr[i].x, 0.0F, (float)chunk_list.arr[i].z }, transform);
		graphics_shader_matrix("model", transform);

		graphics_buffer_draw(chunk_list.arr[i].opaque_buffer);
	}
}