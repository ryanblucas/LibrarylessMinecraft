/*
	world_chunk.c ~ RL
	Minecraft chunk manager
*/

#define WORLD_INTERNAL
#include "world.h"
#include "perlin.h"

#define RADIUS 8

array_list_t chunk_list;

static perlin_state_t perlin_terrain;

void world_chunk_init(unsigned int seed)
{
	chunk_list = mc_list_create(sizeof(struct chunk));
	perlin_terrain = perlin_create_with_seed(seed);
	for (int y = -RADIUS; y < RADIUS; y++)
	{
		for (int x = -RADIUS; x < RADIUS; x++)
		{
			world_chunk_create(x * 16, y * 16);
		}
	}
}

void world_chunk_destroy(void)
{
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->opaque_buffer);
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->liquid_buffer);
	}
	mc_list_destroy(&chunk_list);
	perlin_delete(&perlin_terrain);
}

struct chunk* world_chunk_create(int x_o, int z_o)
{
	x_o = ROUND_DOWN(x_o, CHUNK_WX);
	z_o = ROUND_DOWN(z_o, CHUNK_WZ);

	if (world_chunk_get(x_o, z_o))
	{
		return world_chunk_get(x_o, z_o);
	}

	int res = mc_list_add(chunk_list, mc_list_count(chunk_list), NULL, sizeof(struct chunk));
	struct chunk* next = MC_LIST_CAST_GET(chunk_list, res, struct chunk);

	next->x = x_o;
	next->z = z_o;

	memset(next->arr, 0, sizeof next->arr);

	for (int i = 0; i < CHUNK_FLOOR_BLOCK_COUNT; i++)
	{
		int slice_height = perlin_brownian_at(perlin_terrain, CHUNK_FX(i) + next->x, CHUNK_FZ(i) + next->z, 6) * 32;
		slice_height += 64;
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

	for (int i = 0; i < CHUNK_BLOCK_COUNT; i++)
	{
		if (next->arr[i] == BLOCK_AIR)
		{
			continue;
		}
		double val = perlin_at_3d(perlin_terrain, (CHUNK_FX(i) + next->x) / 64.0, CHUNK_FY(i) / 64.0, (CHUNK_FZ(i) + next->z) / 64.0);
		if (val > 0.4 && val < 0.6)
		{
			next->arr[i] = BLOCK_AIR;
		}
	}

	next->dirty_mask = OPAQUE_BIT;
	next->opaque_buffer = graphics_buffer_create(NULL, 0, VERTEX_BLOCK);
	next->liquid_buffer = graphics_buffer_create(NULL, 0, VERTEX_BLOCK);
	return next;
}

struct chunk* world_chunk_get(int x, int z)
{
	x = ROUND_DOWN(x, CHUNK_WX);
	z = ROUND_DOWN(z, CHUNK_WZ);
	struct chunk* block_vertex_list = mc_list_array(chunk_list);
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		if (block_vertex_list[i].x == x && block_vertex_list[i].z == z)
		{
			return &block_vertex_list[i];
		}
	}
	return NULL;
}