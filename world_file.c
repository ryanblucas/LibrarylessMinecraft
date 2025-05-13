/*
	world_file.c ~ RL
	Saves and reads chunks from disk
*/

#define WORLD_INTERNAL
#include "world.h"
#include <stdint.h>
#include <Windows.h>

#define START_RADIUS 2
#define WORLD_DIRECTORY "worlds"
#define WORLD_FILE WORLD_DIRECTORY "/game.wrld"

struct file_header
{
	int32_t player_x, player_y, player_z;
	uint32_t seed;
};

struct file_chunk
{
	int32_t x, z;
	block_type_t blocks[CHUNK_BLOCK_COUNT];
};

static inline FILE* world_file_try_open_world(const char* mode)
{
	FILE* file = fopen(WORLD_FILE, mode);
	if (file)
	{
		/* either created file or opened pre-existing file */
		return file;
	}
	/* directory doesn't exist */
	mc_panic_if(!CreateDirectoryA(WORLD_DIRECTORY, NULL), "Failed to create worlds directory");
	return fopen(WORLD_FILE, mode);
}

void world_file_load_world(unsigned int fallback_seed)
{
	long size;
	char* world = mc_read_file_binary("worlds/game.wrld", &size);
	if (!world)
	{
		printf("No world exists, creating new one...\n");
		world_chunk_init(fallback_seed);
		for (int y = -START_RADIUS; y < START_RADIUS; y++)
		{
			for (int x = -START_RADIUS; x < START_RADIUS; x++)
			{
				world_chunk_create(x * 16, y * 16);
			}
		}

		block_coords_t spawn = { 0, CHUNK_WY - 1, 0 };
		for (; world_block_get(spawn) == BLOCK_AIR; spawn.y--);
		spawn.y += 2;
		player.hitbox = aabb_set_center(player.hitbox, block_coords_to_vector(spawn));

		world_file_save_current();
		return;
	}

	printf("Opening pre-existing world...\n");
	struct file_header* header = (struct file_header*)world;
	struct file_chunk* chunks = (struct file_chunk*)(header + 1);
	int chunk_count = (size - sizeof(struct file_header)) / sizeof(struct file_chunk);

	world_chunk_init(header->seed);

	for (int i = 0; i < chunk_count; i++)
	{
		if (chunks[i].x - RADIUS_BLOCKS < header->player_x && chunks[i].x + RADIUS_BLOCKS > header->player_x &&
			chunks[i].z - RADIUS_BLOCKS < header->player_z && chunks[i].z + RADIUS_BLOCKS > header->player_z)
		{
			world_chunk_add(chunks[i].x, chunks[i].z, chunks[i].blocks);
		}
	}
	
	player.hitbox = aabb_set_center(player.hitbox, block_coords_to_vector((block_coords_t) { header->player_x, header->player_y, header->player_z }));

	free(world);
}

static void world_file_save_chunk_internal(FILE* file, int x, int z)
{
	struct chunk* chunk = world_chunk_get(x, z);
	assert(chunk);
	x = ROUND_DOWN(x, CHUNK_WX);
	z = ROUND_DOWN(z, CHUNK_WZ);
	fseek(file, sizeof(struct file_header), SEEK_SET);
	while (fgetc(file) != EOF)
	{
		int cx = -1, cz = -1;
		fread(&cx, 1, sizeof cx, file);
		fread(&cz, 1, sizeof cz, file);
		if (cx == x && cz == z)
		{
			fseek(file, 0, SEEK_CUR);
			fwrite(chunk->arr, 1, sizeof chunk->arr, file);
		}
		fseek(file, CHUNK_BLOCK_COUNT * sizeof(block_type_t), SEEK_CUR);
	}
	fseek(file, 0, SEEK_CUR);
	fwrite(&x, 1, sizeof x, file);
	fwrite(&z, 1, sizeof z, file);
	fwrite(chunk->arr, 1, sizeof chunk->arr, file);
}

static void world_file_save_header(FILE* file, block_coords_t rounded_pos, uint32_t seed)
{
	fseek(file, 0, SEEK_SET);
	fwrite(&rounded_pos.x, 1, sizeof rounded_pos.x, file);
	fwrite(&rounded_pos.y, 1, sizeof rounded_pos.y, file);
	fwrite(&rounded_pos.z, 1, sizeof rounded_pos.z, file);
	fwrite(&seed, 1, sizeof seed, file);
}

void world_file_save_chunk(int x, int z)
{
#pragma warning( push )
#pragma warning( disable : 6387)
	FILE* file = world_file_try_open_world("ab+");
	mc_panic_if(!file, "Failed to save chunk");

	world_file_save_chunk_internal(file, x, z);

	fclose(file);
#pragma warning( pop ) 
}

void world_file_save_current(void)
{
#pragma warning( push )
#pragma warning( disable : 6387)
	printf("Saving current world...\n");

	FILE* file = world_file_try_open_world("ab+");
	mc_panic_if(!file, "Failed to save chunk");

	world_file_save_header(file, vector_to_block_coords(aabb_get_center(player.hitbox)), world_seed());
	struct chunk* chunks = mc_list_array(chunk_list);
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		world_file_save_chunk_internal(file, chunks[i].x, chunks[i].z);
	}

	fclose(file);
#pragma warning( pop ) 
}

void world_file_delete(void)
{
	printf("Removing current world...\n");
	remove(WORLD_FILE);
}

struct chunk* world_file_find_chunk(int x, int z)
{
	FILE* file = world_file_try_open_world("rb+");
	mc_panic_if(!file, "Failed to open world");

	x = ROUND_DOWN(x, CHUNK_WX);
	z = ROUND_DOWN(z, CHUNK_WZ);
	fseek(file, sizeof(struct file_header), SEEK_SET);
	while (fgetc(file) != EOF)
	{
		int cx = -1, cz = -1;
		fread(&cx, 1, sizeof cx, file);
		fread(&cz, 1, sizeof cz, file);
		if (cx == x && cz == z)
		{
			fseek(file, 0, SEEK_CUR);
			block_type_t chunk[CHUNK_BLOCK_COUNT];
			fread(chunk, 1, CHUNK_BLOCK_COUNT, file);
			fclose(file);

			return world_chunk_add(x, z, chunk);
		}
		fseek(file, CHUNK_BLOCK_COUNT * sizeof(block_type_t), SEEK_CUR);
	}
	fclose(file);
	return NULL;
}