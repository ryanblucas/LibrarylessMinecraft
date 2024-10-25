/*
	world_mesh.c ~ RL
	Generates the mesh for a chunk
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>

enum quad_normal
{
	LEFT = 0b101,
	RIGHT = 0b001,
	UP = 0b110,
	DOWN = 0b010,
	FORWARD = 0b111,
	BACKWARD = 0b011,

	FLIPPED_BIT = 0b100
};

static struct vertex_array_list
{
	size_t reserved, count;
	block_vertex_t array[];
} *list;

static inline void world_mesh_finalize_quad(int type, enum quad_normal normal, block_vertex_t a, block_vertex_t b, block_vertex_t c, block_vertex_t d)
{
	if (normal & FLIPPED_BIT)
	{
		c = SET_BLOCK_VERTEX_TEXTURE(c, 0, 0, type);
		b = SET_BLOCK_VERTEX_TEXTURE(b, 1, 0, type);
		a = SET_BLOCK_VERTEX_TEXTURE(a, 1, 1, type);
		d = SET_BLOCK_VERTEX_TEXTURE(d, 0, 1, type);
	}
	else
	{
		a = SET_BLOCK_VERTEX_TEXTURE(a, 1, 0, type);
		b = SET_BLOCK_VERTEX_TEXTURE(b, 0, 0, type);
		c = SET_BLOCK_VERTEX_TEXTURE(c, 0, 1, type);
		d = SET_BLOCK_VERTEX_TEXTURE(d, 1, 1, type);
	}

	size_t curr = list->count;
	list->count += 6;
	list->array[curr + 0] = a;
	list->array[curr + 1] = b;
	list->array[curr + 2] = c;
	list->array[curr + 3] = c;
	list->array[curr + 4] = d;
	list->array[curr + 5] = a;

	if (list->count + 6 >= list->reserved)
	{
		size_t old_size = sizeof * list + sizeof * list->array * list->reserved;
		struct vertex_array_list* prev = list;
		list = mc_malloc(old_size + sizeof * list->array * list->reserved);
		memcpy(list, prev, old_size);
		list->reserved *= 2;
		free(prev);
	}
}

static void world_mesh_quad(int mask, int type, enum quad_normal normal)
{
	block_vertex_t a, b, c, d;
	switch (normal)
	{
	case LEFT:
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask) + 1,	CHUNK_Z(mask) + 1 );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask) + 1,	CHUNK_Z(mask) );
		break;
	case RIGHT:
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask) + 1,	CHUNK_Z(mask) + 1 );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask) + 1,	CHUNK_Z(mask) );
		break;
	case UP:
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) );
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		break;
	case DOWN:
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask) + 1,	CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask) + 1,	CHUNK_Z(mask) );
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask) + 1,	CHUNK_Z(mask) + 1 );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask) + 1,	CHUNK_Z(mask) + 1 );
		break;
	case FORWARD:
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask) + 1,	CHUNK_Z(mask) + 1 );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask) + 1,	CHUNK_Z(mask) + 1 );
		break;
	case BACKWARD:
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) );
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask) + 1,	CHUNK_Z(mask) );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask) + 1,	CHUNK_Z(mask) );
		break;
	default:
		assert(false);
		break;
	}

	world_mesh_finalize_quad(type, normal, a, b, c, d);
}

static void world_mesh_flowing_water(int mask, int strength, enum quad_normal normal)
{
	block_vertex_t a, b, c, d;
	strength--;
	switch (normal)
	{
	case LEFT:
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		a = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1,	strength);
		d = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask),		strength);
		break;
	case RIGHT:
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		c = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1,	strength);
		d = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask),		strength);
		break;
	case UP:
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) );
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		d = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		break;
	case DOWN:
		a = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask),		strength);
		b = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask),		strength);
		c = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1,	strength);
		d = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1,	strength);
		break;
	case FORWARD:
		c = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1 );
		a = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) + 1,	strength);
		d = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) + 1,	strength);
		break;
	case BACKWARD:
		a = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask) );
		b = CREATE_BLOCK_VERTEX_POS( CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask) );
		c = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask) + 1,	CHUNK_Y(mask),		CHUNK_Z(mask),		strength);
		d = CREATE_LIQUID_VERTEX_POS(CHUNK_X(mask),		CHUNK_Y(mask),		CHUNK_Z(mask),		strength);
		break;
	default:
		assert(false);
		break;
	}

	world_mesh_finalize_quad(BLOCK_WATER - 1, normal, a, b, c, d);
}

static inline const block_type_t* world_chunk_get_array_or_default(int x, int z)
{
	struct chunk* chunk = world_chunk_get(x, z);
	if (chunk)
	{
		return chunk->arr;
	}

	static block_type_t def[CHUNK_BLOCK_COUNT];
	return def;
}

static void world_chunk_clean_opaque(struct chunk* chunk)
{
	const block_type_t* left = world_chunk_get_array_or_default(chunk->x - CHUNK_WX, chunk->z),
		*right = world_chunk_get_array_or_default(chunk->x + CHUNK_WX, chunk->z),
		*backward = world_chunk_get_array_or_default(chunk->x, chunk->z - CHUNK_WZ),
		*forward = world_chunk_get_array_or_default(chunk->x, chunk->z + CHUNK_WZ);
	const block_type_t* arr = chunk->arr;
	for (int mask = 0; mask < CHUNK_BLOCK_COUNT; mask++)
	{
		int x = CHUNK_X(mask), y = CHUNK_Y(mask), z = CHUNK_Z(mask);
		block_type_t curr = chunk->arr[mask];
		if (!IS_SOLID(curr))
		{
			continue;
		}
		curr--;
		if ((x == 0 && !IS_SOLID(CHUNK_AT(left, 15, y, z))) || !IS_SOLID(CHUNK_AT(arr, x - 1, y, z)))
		{
			world_mesh_quad(mask, curr, LEFT);
		}
		if ((x == CHUNK_WX - 1 && !IS_SOLID(CHUNK_AT(right, 0, y, z))) || !IS_SOLID(CHUNK_AT(arr, x + 1, y, z)))
		{
			world_mesh_quad(mask, curr, RIGHT);
		}
		if (y == 0 || !IS_SOLID(CHUNK_AT(arr, x, y - 1, z)))
		{
			world_mesh_quad(mask, curr, UP);
		}
		if (y == CHUNK_WY - 1 || !IS_SOLID(CHUNK_AT(arr, x, y + 1, z)))
		{
			world_mesh_quad(mask, curr, DOWN);
		}
		if ((z == 0 && !IS_SOLID(CHUNK_AT(backward, x, y, 15))) || !IS_SOLID(CHUNK_AT(arr, x, y, z - 1)))
		{
			world_mesh_quad(mask, curr, BACKWARD);
		}
		if ((z == CHUNK_WZ - 1 && !IS_SOLID(CHUNK_AT(forward, x, y, 0))) || !IS_SOLID(CHUNK_AT(arr, x, y, z + 1)))
		{
			world_mesh_quad(mask, curr, FORWARD);
		}
	}

	graphics_buffer_modify(chunk->opaque_buffer, list->array, list->count);
	list->count = 0;
	chunk->dirty_mask ^= OPAQUE_BIT;
}

static void world_chunk_clean_liquid(struct chunk* chunk)
{
	for (int i = 0; i < mc_list_count(chunk->flowing_liquid); i++)
	{
		struct liquid* liquid = MC_LIST_CAST_GET(chunk->flowing_liquid, i, struct liquid);
		int mask = CHUNK_INDEX_OF(liquid->position.x - chunk->x, liquid->position.y, liquid->position.z - chunk->z);
		int strength = liquid->came_from_above ? 8 : liquid->strength;
		world_mesh_flowing_water(mask, strength, LEFT);
		world_mesh_flowing_water(mask, strength, RIGHT);
		world_mesh_flowing_water(mask, strength, BACKWARD);
		world_mesh_flowing_water(mask, strength, FORWARD);
		world_mesh_flowing_water(mask, strength, UP);
		world_mesh_flowing_water(mask, strength, DOWN);
	}

	graphics_buffer_modify(chunk->liquid_buffer, list->array, list->count);
	list->count = 0;
	chunk->dirty_mask ^= LIQUID_BIT;
}

void world_chunk_clean_mesh(int index)
{
	/*	This is NOT a greedy mesher. That's because texture wrapping is impossible with texture atlases, but using
		texture arrays with a greedy mesher fixes that problem AND the mipmapping problem. Just a thought, TO DO */

	if (!list)
	{
		list = mc_malloc(sizeof * list + sizeof * list->array * 36);
		list->reserved = 36;
		list->count = 0;
	}

	struct chunk* chunk = MC_LIST_CAST_GET(chunk_list, index, struct chunk);
	if (chunk->dirty_mask & OPAQUE_BIT)
	{
		world_chunk_clean_opaque(chunk);
	}
	if (chunk->dirty_mask & LIQUID_BIT)
	{
		world_chunk_clean_liquid(chunk);
	}
}