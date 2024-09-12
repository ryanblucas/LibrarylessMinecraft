/*
	world.c ~ RL
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>
#include "entity.h"
#include "graphics.h"

struct chunk
{
	bool is_dirty;
	vertex_buffer_t opaque_buffer;

	/* Since most of this is air anyways, this could set a variable height to save memory, TO DO */
	block_type_t arr[CHUNK_BLOCK_COUNT];
} chunk;

entity_t player;

void world_init(void)
{
	for (int i = 0; i < CHUNK_FLOOR_BLOCK_COUNT; i++)
	{
		int x = CHUNK_X(i), z = CHUNK_Z(i);
		world_block_set((block_coords_t) { x, 128, z }, BLOCK_GRASS);
		for (int j = 127; j >= 125; j--)
		{
			world_block_set((block_coords_t) { x, j, z }, BLOCK_DIRT);
		}
		for (int j = 124; j >= 0; j--)
		{
			world_block_set((block_coords_t) { x, j, z }, BLOCK_STONE);
		}
	}
	chunk.opaque_buffer = graphics_buffer_create(NULL, 0);
	entity_player_init(&player);
	player.hitbox = aabb_set_center(player.hitbox, (vector3_t) { 8, 130, 8 });
}

void world_destroy(void)
{
	graphics_buffer_delete(&chunk.opaque_buffer);
}

struct ray_state
{
	ray_t curr;
	float len;
	vector3_t inv_dir, dir, start;
};

void world_raycast_loop(block_coords_t i, void* user)
{
	if (CHUNK_AT(chunk.arr, i.x, i.y, i.z) == BLOCK_AIR)
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
	_min = vector_to_block_coords(vector3_max(vector3_min(v_min, v_max), (vector3_t){ 0.0F, 0.0F, 0.0F }));
	_max = vector_to_block_coords(vector3_min(vector3_max(v_min, v_max), (vector3_t){ CHUNK_WX - 1, CHUNK_WY - 1, CHUNK_WZ - 1 }));

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
	if (IS_INVALID_BLOCK_COORDS(coords))
	{
		return BLOCK_AIR;
	}

	return chunk.arr[CHUNK_INDEX_OF(coords.x, coords.y, coords.z)];
}

void world_block_set(block_coords_t coords, block_type_t type)
{
	if (IS_INVALID_BLOCK_COORDS(coords))
	{
		return;
	}

	chunk.arr[CHUNK_INDEX_OF(coords.x, coords.y, coords.z)] = type;
	chunk.is_dirty = true;
}

void world_update(float delta)
{
	entity_player_update(&player, delta);
	entity_move(&player, player.velocity);
}

void world_render(float delta)
{
	if (chunk.is_dirty)
	{
		world_mesh_chunk(chunk.opaque_buffer, chunk.arr);
		chunk.is_dirty = false;
	}

	graphics_buffer_draw(chunk.opaque_buffer);
}