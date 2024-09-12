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

struct vertex_array_list
{
	size_t reserved, count;
	vertex_t array[];
};

static struct vertex_array_list* world_mesh_quad(struct vertex_array_list* list, int mask, block_type_t type, enum quad_normal normal)
{
	vertex_t a, b, c, d;
	switch (normal)
	{
	case LEFT:
		c = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		a = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) };
		break;
	case RIGHT:
		a = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		c = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) };
		break;
	case UP:
		c = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) };
		a = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		break;
	case DOWN:
		a = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) };
		c = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) + 1 };
		break;
	case FORWARD:
		c = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) + 1 };
		a = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) + 1 };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) + 1 };
		break;
	case BACKWARD:
		a = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask),		CHUNK_FZ(mask) };
		b = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask),		CHUNK_FZ(mask) };
		c = (vertex_t){ CHUNK_FX(mask) + 1,	CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) };
		d = (vertex_t){ CHUNK_FX(mask),		CHUNK_FY(mask) + 1,	CHUNK_FZ(mask) };
		break;
	default:
		assert(false);
		break;
	}

	if (normal & FLIPPED_BIT)
	{
		/* -1 for air */
		c.tx = (float)(type - 1) / (BLOCK_COUNT - 1);
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
		a.tx = (float)(type) / (BLOCK_COUNT - 1);
		a.ty = 0.0F;
		c.tx = a.tx - 1.0F / (BLOCK_COUNT - 1);
		c.ty = 1.0F;
		b.tx = c.tx;
		b.ty = 0.0F;
		d.tx = a.tx;
		d.ty = 1.0F;
	}

	list->array[list->count++] = a;
	list->array[list->count++] = b;
	list->array[list->count++] = c;
	list->array[list->count++] = c;
	list->array[list->count++] = d;
	list->array[list->count++] = a;

	if (list->count + 6 >= list->reserved) /* +6 for next quad */
	{
		size_t old_size = sizeof * list + sizeof * list->array * list->reserved;
		list->reserved *= 2;
		struct vertex_array_list* prev = list;
		list = mc_malloc(old_size + sizeof * list->array * list->reserved);
		memcpy(list, prev, old_size);
		free(prev);
	}

	return list;
}

void world_mesh_chunk(vertex_buffer_t out, const block_type_t chunk[CHUNK_BLOCK_COUNT])
{
	/*	This is NOT a greedy mesher. That's because texture wrapping is impossible with texture atlases, but using
		texture arrays with a greedy mesher fixes that problem AND the mipmapping problem. Just a thought, TO DO */

	static struct vertex_array_list* val;
	if (!val)
	{
		val = mc_malloc(sizeof * val + sizeof * val->array * 36);
		val->reserved = 36;
	}
	val->count = 0;

	for (int mask = 0; mask < CHUNK_BLOCK_COUNT; mask++)
	{
		int x = CHUNK_X(mask), y = CHUNK_Y(mask), z = CHUNK_Z(mask);
		block_type_t curr = chunk[mask];
		if (curr == BLOCK_AIR)
		{
			continue;
		}
		if (x == 0 || BLOCK_AIR == CHUNK_AT(chunk, x - 1, y, z))
		{
			val = world_mesh_quad(val, mask, curr, LEFT);
		}
		if (x == CHUNK_WX - 1 || BLOCK_AIR == CHUNK_AT(chunk, x + 1, y, z))
		{
			val = world_mesh_quad(val, mask, curr, RIGHT);
		}
		if (y == 0 || BLOCK_AIR == CHUNK_AT(chunk, x, y - 1, z))
		{
			val = world_mesh_quad(val, mask, curr, UP);
		}
		if (y == CHUNK_WY - 1 || BLOCK_AIR == CHUNK_AT(chunk, x, y + 1, z))
		{
			val = world_mesh_quad(val, mask, curr, DOWN);
		}
		if (z == 0 || BLOCK_AIR == CHUNK_AT(chunk, x, y, z - 1))
		{
			val = world_mesh_quad(val, mask, curr, BACKWARD);
		}
		if (z == CHUNK_WZ - 1 || BLOCK_AIR == CHUNK_AT(chunk, x, y, z + 1))
		{
			val = world_mesh_quad(val, mask, curr, FORWARD);
		}
	}

	graphics_buffer_modify(out, val->array, val->count);
}