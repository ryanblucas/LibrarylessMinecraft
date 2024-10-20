/*
	world_chunk.c ~ RL
	Minecraft chunk manager
*/

#define WORLD_INTERNAL
#include "world.h"
#include "perlin.h"

#define ROUND_DOWN(c, m) (((c) < 0 ? -((-(c) - 1 + (m)) / (m)) : (c) / (m)) * (m))

array_list_t chunk_list;

void world_chunk_init(void)
{
	chunk_list = mc_list_create(sizeof(struct chunk));
	world_chunk_create(0, 0);
	/*
	for (int x = -8; x <= 8; x++)
	{
		for (int y = -8; y <= 8; y++)
		{
			world_chunk_create(x * 16, y * 16);
		}
	}*/
}

void world_chunk_destroy(void)
{
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->opaque_buffer);
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->liquid_buffer);
	}
	mc_list_destroy(&chunk_list);
}

struct chunk* world_chunk_create(int x_o, int z_o)
{
	x_o = ROUND_DOWN(x_o, CHUNK_WX);
	z_o = ROUND_DOWN(z_o, CHUNK_WZ);

	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		struct chunk* curr = MC_LIST_CAST_GET(chunk_list, i, struct chunk);
		if (curr->x == x_o && curr->z == z_o)
		{
			return curr;
		}
	}

	int res = mc_list_add(chunk_list, mc_list_count(chunk_list), NULL, sizeof(struct chunk));
	struct chunk* next = MC_LIST_CAST_GET(chunk_list, res, struct chunk);

	next->x = x_o;
	next->z = z_o;
	next->flowing_liquid = mc_list_create(sizeof(liquid_t));

	/*
	for (int i = 0; i < CHUNK_FLOOR_BLOCK_COUNT; i++)
	{
		int slice_height = perlin_brownian_at(CHUNK_FX(i) + next->x, CHUNK_FZ(i) + next->z, 6, 1.0, 0.005) * 128.0;
		slice_height = max(min(slice_height, CHUNK_WY - 1), 0);
		CHUNK_AT(next->arr, CHUNK_X(i), slice_height--, CHUNK_Z(i)) = BLOCK_GRASS;
		for (int j = 1; j < 4 && slice_height >= 0; j++, slice_height--)
		{
			CHUNK_AT(next->arr, CHUNK_X(i), slice_height, CHUNK_Z(i)) = BLOCK_DIRT;
		}
		while (slice_height >= 0)
		{
			CHUNK_AT(next->arr, CHUNK_X(i), slice_height--, CHUNK_Z(i)) = BLOCK_STONE;
		}
	}
	*/

	memset(next->arr, 0, sizeof next->arr);
	CHUNK_AT(next->arr, 7, 128, 7) = BLOCK_GRASS;

	next->dirty_mask = OPAQUE_BIT;
	next->opaque_buffer = graphics_buffer_create(NULL, 0, BLOCK_VERTEX);
	next->liquid_buffer = graphics_buffer_create(NULL, 0, BLOCK_VERTEX);
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