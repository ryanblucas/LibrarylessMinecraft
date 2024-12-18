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

static array_list_t update_list;
static int ticks;

entity_t player;

void world_init(void)
{
	update_list = mc_list_create(sizeof(block_coords_t));
	world_chunk_init();
	world_render_init();

	entity_player_init(&player);
}

void world_destroy(void)
{
	world_chunk_destroy();
	world_render_destroy();
	mc_list_destroy(&update_list);
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
	return mc_map_get(chunk->flowing_liquid, mc_hash(&coords, sizeof coords), NULL, 0);
}

static void world_liquid_remove(block_coords_t coords)
{
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk)
	{
		return;
	}
	mc_map_remove(chunk->flowing_liquid, mc_hash(&coords, sizeof coords), NULL, 0);
	chunk->dirty_mask |= LIQUID_BIT;
}

static liquid_t* world_liquid_add(block_coords_t coords, vector3_t push, int strength)
{
	if (world_liquid_strength(coords) >= strength)
	{
		return world_liquid_get(coords);
	}

	world_liquid_remove(coords);
	struct chunk* chunk = world_chunk_get(coords.x, coords.z);
	if (!chunk || IS_SOLID(world_block_get(coords)))
	{
		return NULL;
	}
	liquid_t to_add = { .position = coords, .push = push, .strength = strength, .tick_count = ticks };
	to_add.came_from_above = !!world_liquid_get((block_coords_t) { coords.x, coords.y + 1, coords.z });

	hash_t key = mc_hash(&coords, sizeof coords);
	mc_map_add(chunk->flowing_liquid, key, &to_add, sizeof to_add);
	chunk->dirty_mask |= LIQUID_BIT;
	world_block_update(coords);
	return mc_map_get(chunk->flowing_liquid, key, NULL, 0);
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
	world_liquid_add(down, (vector3_t) { 0 }, WATER_STRENGTH);
}

static void world_block_tick(void)
{
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

	world_block_update((block_coords_t) { coords.x, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x - 1, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x + 1, coords.y, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y - 1, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y + 1, coords.z });
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z - 1 });
	world_block_update((block_coords_t) { coords.x, coords.y, coords.z + 1 });

	chunk->arr[CHUNK_INDEX_OF(coords.x - chunk->x, coords.y, coords.z - chunk->z)] = type;
	chunk->dirty_mask |= OPAQUE_BIT;

	struct chunk* neighbor = NULL;
	if (coords.x - chunk->x == 0)
	{
		neighbor = world_chunk_get(coords.x - 1, coords.z);
	}
	else if (coords.x - chunk->x == 15)
	{
		neighbor = world_chunk_get(coords.x + 1, coords.z);
	}
	else if (coords.z - chunk->z == 0)
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
		neighbor->dirty_mask |= OPAQUE_BIT;
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

	float angle = atan2f(push.z, push.x);
	fprintf(stream, "(%i, %i, %i), chunk %i (%i, %i). Block name \"%s,\" block id: %i. Liquid strength: %i, push: (%f, %f, %f), angle: %f deg\n", 
		coords.x, coords.y, coords.z, i, chunk->x, chunk->z, name, id, world_liquid_strength(coords), push.x, push.y, push.z, RADIANS_TO_DEGREES(angle));
}

static bool world_liquid_move_player(const map_t map, hash_t key, void* value, void* user)
{
	float delta = *(float*)user;
	liquid_t* curr = (liquid_t*)value;
	if (aabb_collides_aabb(world_liquid_to_aabb(*curr), player.hitbox))
	{
		entity_move(&player, vector3_mul_scalar(curr->push, delta));
	}
	return true;
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

	struct chunk* player_chunk = world_chunk_get((int)aabb_get_center(player.hitbox).x, (int)aabb_get_center(player.hitbox).z);
	if (player_chunk && !entity_player_is_noclipping(&player))
	{
		mc_map_iterate(player_chunk->flowing_liquid, world_liquid_move_player, &delta);
	}

	ticks++;
}