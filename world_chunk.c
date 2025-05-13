/*
	world_chunk.c ~ RL
	Minecraft chunk manager
*/

#define WORLD_INTERNAL
#include "world.h"
#include "perlin.h"

#define BLOCK_RADIUS (RADIUS * 16)
#define PERLIN_MODIFIER (1.0 / 16.0)
#define TWISTINESS (1.0F / 64.0F)

#define MAX_CHUNKS_PER_TICK 2

array_list_t chunk_list;

static hash_set_t chunks_to_generate;

/* The problem is that if the chunk already exists, it doesn't dig into it. */
static array_list_t cave_blocks;
static perlin_state_t perlin_terrain;

void world_chunk_init(unsigned int seed)
{
	chunk_list = mc_list_create(sizeof(struct chunk));
	cave_blocks = mc_list_create(sizeof(block_coords_t));
	perlin_terrain = perlin_create_with_seed(50);
	chunks_to_generate = mc_set_create(sizeof(block_coords_t));
}

void world_chunk_destroy(void)
{
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->opaque_buffer);
		graphics_buffer_delete(&MC_LIST_CAST_GET(chunk_list, i, struct chunk)->liquid_buffer);
	}
	mc_list_destroy(&chunk_list);
	mc_list_destroy(&cave_blocks);
	perlin_delete(&perlin_terrain);

	mc_set_destroy(&chunks_to_generate);
}

static void world_chunk_spawn_vain(struct chunk* curr, block_type_t type, block_coords_t pos, int size_min, int size_max)
{
	if (CHUNK_AT(curr->arr, pos.x, pos.y, pos.z) != BLOCK_STONE)
	{
		return;
	}
	int size = rand() % (size_max - size_min) + size_min;
	for (int i = 0; i < size; i++)
	{
		CHUNK_AT(curr->arr, pos.x, pos.y, pos.z) = type;
		block_coords_t begin = pos;
		for (int j = 0; is_block_coords_equal(begin, pos); j++)
		{
			if (j > 8)
			{
				return;
			}
			switch (rand() % 6)
			{
			case 0: /* left */
				if (pos.x - 1 >= 0 && CHUNK_AT(curr->arr, pos.x - 1, pos.y, pos.z) != type) pos.x--;
				break;
			case 1: /* right */
				if (pos.x + 1 < CHUNK_WX && CHUNK_AT(curr->arr, pos.x + 1, pos.y, pos.z) != type) pos.x++;
				break;
			case 2: /* forward */
				if (pos.z - 1 >= 0 && CHUNK_AT(curr->arr, pos.x, pos.y, pos.z - 1) != type) pos.z--;
				break;
			case 3: /* backward */
				if (pos.z + 1 < CHUNK_WZ && CHUNK_AT(curr->arr, pos.x, pos.y, pos.z + 1) != type) pos.z++;
				break;
			case 4: /* up */
				if (pos.y + 1 < CHUNK_WY && CHUNK_AT(curr->arr, pos.x, pos.y + 1, pos.z) != type) pos.y++;
				break;
			case 5: /* down */
				if (pos.y - 1 >= 0 && CHUNK_AT(curr->arr, pos.x, pos.y - 1, pos.z) != type) pos.y--;
				break;
			}
		}
	}
}

static void world_chunk_spawn_ores(struct chunk* next)
{
	for (int i = rand() % 20 + 10; i >= 0; i--)
	{
		world_chunk_spawn_vain(next, BLOCK_ORE_COAL, (block_coords_t) { rand() % CHUNK_WX, rand() % 60 + 2, rand() % CHUNK_WZ }, 2, 10);
	}
	for (int i = rand() % 16 + 4; i >= 0; i--)
	{
		world_chunk_spawn_vain(next, BLOCK_ORE_IRON, (block_coords_t) { rand() % CHUNK_WX, rand() % 50 + 2, rand() % CHUNK_WZ }, 1, 6);
	}
	for (int i = rand() % 6 + 3; i >= 0; i--)
	{
		world_chunk_spawn_vain(next, BLOCK_ORE_GOLD, (block_coords_t) { rand() % CHUNK_WX, rand() % 30 + 2, rand() % CHUNK_WZ }, 1, 6);
	}
	world_chunk_spawn_vain(next, BLOCK_ORE_DIAMOND, (block_coords_t) { rand() % CHUNK_WX, rand() % 22 + 2, rand() % CHUNK_WZ }, 1, 8);
}

static void world_chunk_spawn_terrain(struct chunk* next)
{
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
}

static inline void world_chunk_square(struct chunk* next, int x, int y, int z, int radius, block_type_t type)
{
	for (int xo = max(0, x - radius); xo <= min(CHUNK_WX - 1, x + radius); xo++)
	{
		for (int zo = max(0, z - radius); zo <= min(CHUNK_WZ - 1, z + radius); zo++)
		{
			if (CHUNK_AT(next->arr, xo, y, zo) == BLOCK_AIR)
			{
				CHUNK_AT(next->arr, xo, y, zo) = BLOCK_LEAVES;
			}
		}
	}
}

static void world_chunk_spawn_trees(struct chunk* next)
{
	int count = rand() % 4;
	for (int i = 0; i < count; i++)
	{
		int x = 2 + rand() % (CHUNK_WX - 4), z = 2 + rand() % (CHUNK_WZ - 4);
		int y;
		for (y = CHUNK_WY - 1; !IS_SOLID(CHUNK_AT(next->arr, x, y, z)); y--);
		if (CHUNK_AT(next->arr, x, y, z) != BLOCK_GRASS || y >= 192)
		{
			continue;
		}
		CHUNK_AT(next->arr, x, y, z) = BLOCK_DIRT;
		y++;
		int len = rand() % 3 + 4;
		for (int j = 0; j < len; j++)
		{
			CHUNK_AT(next->arr, x, y + j, z) = BLOCK_LOG;
		}
		for (int yo = len - 2; yo < len; yo++)
		{
			world_chunk_square(next, x, y + yo, z, 2, BLOCK_LEAVES);
		}
		for (int yo = len; yo < len + 2; yo++)
		{
			world_chunk_square(next, x, y + yo, z, 1, BLOCK_LEAVES);
		}
	}
}

static inline void world_chunk_remove_sphere(block_coords_t pos, int radius)
{
	vector3_t fpos = block_coords_to_vector(pos);
	int radius_squared = radius * radius;

	int cx = INT_MIN, cz = INT_MIN;
	bool chunk_exists = false;

	for (int x = pos.x - radius; x < pos.x + radius; x++)
	{
		for (int y = pos.y - radius; y < pos.y + radius; y++)
		{
			for (int z = pos.z - radius; z < pos.z + radius; z++)
			{
				if (vector3_distance_squared(fpos, (vector3_t) { x, y, z }) <= radius_squared)
				{
					if (ROUND_DOWN(cx, 16) != ROUND_DOWN(x, 16) || ROUND_DOWN(cz, 16) != ROUND_DOWN(z, 16))
					{
						struct chunk* chunk = world_chunk_get(x, z);
						chunk_exists = !!chunk && !chunk->generating;
						cx = x;
						cz = z;
					}

					if (y >= 0 && y < CHUNK_WY && !chunk_exists)
					{
						block_coords_t curr = (block_coords_t){ x, y, z };
						mc_list_add(cave_blocks, mc_list_count(cave_blocks), &curr, sizeof curr);
					}
				}
			}
		}
	}
}

static void world_chunk_worm(struct chunk* next)
{
	vector3_t chunk_pos = { next->x, 0, next->z };
	vector3_t seg_pos = { BLOCK_RADIUS - rand() % (BLOCK_RADIUS * 2), rand() % (CHUNK_WY / 3), BLOCK_RADIUS - rand() % (BLOCK_RADIUS * 2) };
	vector3_t noise_pos = { 7.0 / 2048.0, 1163.0 / 2048.0, 409.0 / 2048.0 };
	int segments = rand() % 64 + 64;
	for (int i = 0; i < segments; i++)
	{
		double noiseX = perlin_at_3d(perlin_terrain, noise_pos.x + (i * TWISTINESS), noise_pos.y, noise_pos.z),
			noiseY = perlin_at_3d(perlin_terrain, noise_pos.x, noise_pos.y + (i * TWISTINESS), noise_pos.z),
			noiseZ = perlin_at_3d(perlin_terrain, noise_pos.x, noise_pos.y, noise_pos.z + (i * TWISTINESS));
		vector3_t offset = { cosf(noiseX * 2.0 * M_PI), sinf(noiseY * 0.25 * M_PI), sinf(noiseZ * 2.0 * M_PI) };

		/* slow way of doing it */
		world_chunk_remove_sphere(vector_to_block_coords(vector3_add(seg_pos, chunk_pos)), 3);

		offset.y = -fabsf(offset.y);
		seg_pos = vector3_add(seg_pos, offset);

		if (seg_pos.y <= 7)
		{
			break;
		}
	}
}

static void world_chunk_delete_cave_blocks(struct chunk* next)
{
	const block_coords_t* arr = mc_list_array(cave_blocks);
	for (int i = mc_list_count(cave_blocks) - 1; i >= 0; i--)
	{
		if (arr[i].x >= next->x && arr[i].x < next->x + CHUNK_WX &&
			arr[i].z >= next->z && arr[i].z < next->z + CHUNK_WZ)
		{
			CHUNK_AT(next->arr, arr[i].x - next->x, arr[i].y, arr[i].z - next->z) = BLOCK_AIR;
			mc_list_remove(cave_blocks, i, NULL, 0);
		}
	}
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

	next->generating = true;

	next->x = x_o;
	next->z = z_o;

	memset(next->arr, 0, sizeof next->arr);

	world_chunk_spawn_terrain(next);
	int cave_count = rand() % 2 + 1;
	for (int i = 0; i < cave_count; i++)
	{
		world_chunk_worm(next);
	}
	world_chunk_delete_cave_blocks(next);
	world_chunk_spawn_ores(next);
	world_chunk_spawn_trees(next);

	next->dirty_mask = OPAQUE_BIT;
	next->opaque_buffer = graphics_buffer_create(NULL, 0, VERTEX_BLOCK);
	next->liquid_buffer = graphics_buffer_create(NULL, 0, VERTEX_BLOCK);

	next->generating = false;
	return next;
}

struct chunk* world_chunk_add(int x, int z, block_type_t chunk[CHUNK_BLOCK_COUNT])
{
	if (world_chunk_get(x, z))
	{
		world_chunk_remove(x, z);
	}

	int res = mc_list_add(chunk_list, mc_list_count(chunk_list), NULL, sizeof(struct chunk));
	struct chunk* next = MC_LIST_CAST_GET(chunk_list, res, struct chunk);

	next->x = ROUND_DOWN(x, CHUNK_WX);
	next->z = ROUND_DOWN(z, CHUNK_WZ);

	memcpy(next->arr, chunk, sizeof * chunk * CHUNK_BLOCK_COUNT);

	next->dirty_mask = OPAQUE_BIT;
	next->opaque_buffer = graphics_buffer_create(NULL, 0, VERTEX_BLOCK);
	next->liquid_buffer = graphics_buffer_create(NULL, 0, VERTEX_BLOCK);

	return next;
}

void world_chunk_remove(int x, int z)
{
	x = ROUND_DOWN(x, CHUNK_WX);
	z = ROUND_DOWN(z, CHUNK_WZ);
	struct chunk* block_vertex_list = mc_list_array(chunk_list);
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		if (block_vertex_list[i].x == x && block_vertex_list[i].z == z)
		{
			graphics_buffer_delete(&block_vertex_list[i].opaque_buffer);
			graphics_buffer_delete(&block_vertex_list[i].liquid_buffer);
			mc_list_remove(chunk_list, i, NULL, sizeof(struct chunk));
			return;
		}
	}
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

struct iterate_state
{
	int left;
	block_coords_t arr[MAX_CHUNKS_PER_TICK];
};

static inline void world_chunk_make_dirty(struct chunk* chunk, int mask)
{
	if (chunk)
	{
		chunk->dirty_mask |= mask;
	}
}

static bool world_chunk_map_iterate(const hash_set_t set, void* value, void* user)
{
	struct iterate_state* state = (struct iterate_state*)user;
	block_coords_t* to_load = (block_coords_t*)value;
	
	/* finding chunk creates it */
	struct chunk* chunk = world_file_find_chunk(to_load->x, to_load->z);
	if (!chunk)
	{
		world_chunk_create(to_load->x, to_load->z);
		state->arr[--state->left] = *to_load;
	}

	world_chunk_make_dirty(world_chunk_get(to_load->x - CHUNK_WX, to_load->z), OPAQUE_BIT | LIQUID_BIT);
	world_chunk_make_dirty(world_chunk_get(to_load->x + CHUNK_WX, to_load->z), OPAQUE_BIT | LIQUID_BIT);
	world_chunk_make_dirty(world_chunk_get(to_load->x, to_load->z - CHUNK_WZ), OPAQUE_BIT | LIQUID_BIT);
	world_chunk_make_dirty(world_chunk_get(to_load->x, to_load->z + CHUNK_WZ), OPAQUE_BIT | LIQUID_BIT);

	return state->left > 0;
}

void world_chunk_update(void)
{
	block_coords_t player_location = vector_to_block_coords(aabb_get_center(player.hitbox));
	player_location.x = ROUND_DOWN(player_location.x, CHUNK_WX);
	player_location.z = ROUND_DOWN(player_location.z, CHUNK_WZ);
	player_location.y = 0;
	for (int i = -RADIUS; i < RADIUS; i++)
	{
		for (int j = -RADIUS; j < RADIUS; j++)
		{
			block_coords_t chunk_pos = player_location;
			chunk_pos.x += i * CHUNK_WX;
			chunk_pos.z += j * CHUNK_WZ;
			if (!world_chunk_get(chunk_pos.x, chunk_pos.z))
			{
				mc_set_add(chunks_to_generate, &chunk_pos, sizeof chunk_pos);
			}
		}
	}
	struct iterate_state state;
	state.left = MAX_CHUNKS_PER_TICK;
	mc_set_iterate(chunks_to_generate, world_chunk_map_iterate, &state);
	for (int i = 0; i < MAX_CHUNKS_PER_TICK; i++)
	{
		mc_set_remove(chunks_to_generate, &state.arr[i], sizeof state.arr[i]);
	}
}

unsigned int world_seed(void)
{
	return perlin_get_seed(perlin_terrain);
}