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

static struct block_vertex_list
{
	size_t reserved, count;
	block_vertex_t array[];
} *block_vertex_list;

static struct vertex_list
{
	size_t reserved, count;
	vertex_t array[];
} *vertex_list;

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

	size_t curr = block_vertex_list->count;
	block_vertex_list->count += 6;
	block_vertex_list->array[curr + 0] = a;
	block_vertex_list->array[curr + 1] = b;
	block_vertex_list->array[curr + 2] = c;
	block_vertex_list->array[curr + 3] = c;
	block_vertex_list->array[curr + 4] = d;
	block_vertex_list->array[curr + 5] = a;

	if (block_vertex_list->count + 6 >= block_vertex_list->reserved)
	{
		size_t old_size = sizeof * block_vertex_list + sizeof * block_vertex_list->array * block_vertex_list->reserved;
		struct block_vertex_list* prev = block_vertex_list;
		block_vertex_list = mc_malloc(old_size + sizeof * block_vertex_list->array * block_vertex_list->reserved);
		memcpy(block_vertex_list, prev, old_size);
		block_vertex_list->reserved *= 2;
		free(prev);
	}
}

static inline const block_type_t* world_chunk_get_array_or_default(int x, int z, const block_type_t* def)
{
	struct chunk* chunk = world_chunk_get(x, z);
	if (chunk)
	{
		return chunk->arr;
	}
	return def;
}

static void world_chunk_clean_opaque(struct chunk* chunk)
{
	static block_type_t def[CHUNK_BLOCK_COUNT];
	const block_type_t* left = world_chunk_get_array_or_default(chunk->x - CHUNK_WX, chunk->z, def),
		*right = world_chunk_get_array_or_default(chunk->x + CHUNK_WX, chunk->z, def),
		*backward = world_chunk_get_array_or_default(chunk->x, chunk->z - CHUNK_WZ, def),
		*forward = world_chunk_get_array_or_default(chunk->x, chunk->z + CHUNK_WZ, def);
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
		if ((x == 0 && !IS_SOLID(CHUNK_AT(left, CHUNK_WX - 1, y, z))) || !IS_SOLID(CHUNK_AT(arr, x - 1, y, z)))
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
		if ((z == 0 && !IS_SOLID(CHUNK_AT(backward, x, y, CHUNK_WZ - 1))) || !IS_SOLID(CHUNK_AT(arr, x, y, z - 1)))
		{
			world_mesh_quad(mask, curr, BACKWARD);
		}
		if ((z == CHUNK_WZ - 1 && !IS_SOLID(CHUNK_AT(forward, x, y, 0))) || !IS_SOLID(CHUNK_AT(arr, x, y, z + 1)))
		{
			world_mesh_quad(mask, curr, FORWARD);
		}
	}

	graphics_buffer_modify(chunk->opaque_buffer, block_vertex_list->array, block_vertex_list->count);
	block_vertex_list->count = 0;
	chunk->dirty_mask ^= OPAQUE_BIT;
}

static void world_mesh_flowing_water(int mask, int strength, enum quad_normal normal)
{
	vertex_t a, b, c, d;
	float s = 1 - ((7 - strength) + 1) / 8.0F;
	switch (normal)
	{
	case LEFT:
		c = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		a = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + s,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + s,	CHUNK_FZ(mask) };
		break;
	case RIGHT:
		a = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		c = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + s,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + s,	CHUNK_FZ(mask) };
		break;
	case UP:
		c = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) };
		a = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		break;
	case DOWN:
		a = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + s,	CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + s,	CHUNK_FZ(mask) };
		c = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + s,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + s,	CHUNK_FZ(mask) + 1 };
		break;
	case FORWARD:
		c = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		a = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + s,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + s,	CHUNK_FZ(mask) + 1 };
		break;
	case BACKWARD:
		a = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) };
		c = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + s,	CHUNK_FZ(mask) };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + s,	CHUNK_FZ(mask) };
		break;
	default:
		return;
	}

	if (normal & FLIPPED_BIT)
	{
		/* -1 for air */
		c.tx = (float)(BLOCK_WATER - 1) / (BLOCK_COUNT - 1);
		c.ty = 0.0F;
		a.tx = c.tx + 1.0F / (BLOCK_COUNT - 1);
		a.ty = 1.0F;
		b.tx = a.tx;
		b.ty = 0.0F;
		d.tx = c.tx;
		d.ty = 1.0F;
	}
	else
	{
		/* -1 for air */
		a.tx = (float)(BLOCK_WATER) / (BLOCK_COUNT - 1);
		a.ty = 0.0F;
		c.tx = a.tx - 1.0F / (BLOCK_COUNT - 1);
		c.ty = 1.0F;
		b.tx = c.tx;
		b.ty = 0.0F;
		d.tx = a.tx;
		d.ty = 1.0F;
	}

	vertex_list->array[vertex_list->count++] = a;
	vertex_list->array[vertex_list->count++] = b;
	vertex_list->array[vertex_list->count++] = c;
	vertex_list->array[vertex_list->count++] = c;
	vertex_list->array[vertex_list->count++] = d;
	vertex_list->array[vertex_list->count++] = a;

	if (vertex_list->count + 6 >= vertex_list->reserved)
	{
		size_t old_size = sizeof * vertex_list + sizeof * vertex_list->array * vertex_list->reserved;
		struct vertex_list* prev = vertex_list;
		vertex_list = mc_malloc(old_size + sizeof * vertex_list->array * vertex_list->reserved);
		memcpy(vertex_list, prev, old_size);
		vertex_list->reserved *= 2;
		free(prev);
	}
}

static void world_chunk_clean_liquid(struct chunk* chunk)
{
	for (int i = 0; i < mc_list_count(chunk->flowing_liquid); i++)
	{
		liquid_t* liquid = MC_LIST_CAST_GET(chunk->flowing_liquid, i, liquid_t);
		int mask = CHUNK_INDEX_OF(liquid->position.x - chunk->x, liquid->position.y, liquid->position.z - chunk->z);
		int strength = liquid->came_from_above ? 8 : liquid->strength;
		world_mesh_flowing_water(mask, strength, LEFT);
		world_mesh_flowing_water(mask, strength, RIGHT);
		world_mesh_flowing_water(mask, strength, BACKWARD);
		world_mesh_flowing_water(mask, strength, FORWARD);
		world_mesh_flowing_water(mask, strength, UP);
		world_mesh_flowing_water(mask, strength, DOWN);
	}

	graphics_buffer_modify(chunk->liquid_buffer, vertex_list->array, vertex_list->count);
	vertex_list->count = 0;
	chunk->dirty_mask ^= LIQUID_BIT;
}

void world_chunk_clean_mesh(int index)
{
	/*	This is NOT a greedy mesher. That's because texture wrapping is impossible with texture atlases, but using
		texture arrays with a greedy mesher fixes that problem AND the mipmapping problem. Just a thought, TO DO */

	if (!block_vertex_list)
	{
		block_vertex_list = mc_malloc(sizeof * block_vertex_list + sizeof * block_vertex_list->array * 36);
		block_vertex_list->reserved = 36;
		block_vertex_list->count = 0;
	}

	if (!vertex_list)
	{
		vertex_list = mc_malloc(sizeof * vertex_list + sizeof * vertex_list->array * 36);
		vertex_list->reserved = 36;
		vertex_list->count = 0;
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