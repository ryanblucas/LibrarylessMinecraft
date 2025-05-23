/*
	world.h ~ RL
*/

#pragma once

#include "entity.h"
#include "graphics.h"
#include "item.h"
#include "util.h"

#define CHUNK_WX 16
#define CHUNK_WY 256
#define CHUNK_WZ 16
#define CHUNK_FLOOR_BLOCK_COUNT	(CHUNK_WX * CHUNK_WZ)
#define CHUNK_BLOCK_COUNT		(CHUNK_WX * CHUNK_WY * CHUNK_WZ)

#define WATER_STRENGTH 7

#define IS_INVALID_BLOCK_COORDS(bc) ((bc).y < 0 || (bc).y >= CHUNK_WY)

typedef struct block_coords
{
	int x, y, z;
} block_coords_t;

typedef struct ray
{
	block_coords_t block;	/* The block this ray hit. Is invalid if the ray never hits any block. */
	vector3_t min;			/* The mininum coordinates of the ray. This is the beginning of the ray's intersection w/ the block it hit. */
	vector3_t max;			/* The maximum coordinates of the ray. This is the end of the ray. */
} ray_t;

extern entity_t player;

typedef void (*world_loop_callback_t)(block_coords_t, void*);

extern inline block_coords_t vector_to_block_coords(vector3_t vec)
{
	return (block_coords_t) { (int)vec.x, (int)vec.y, (int)vec.z };
}

extern inline vector3_t block_coords_to_vector(block_coords_t bc)
{
	return (vector3_t) { (float)bc.x, (float)bc.y, (float)bc.z };
}

extern inline bool is_block_coords_equal(block_coords_t a, block_coords_t b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

void world_init(void);
void world_destroy(void);
void world_generate(unsigned int seed);

/* fetch game tick count */
int world_ticks(void);

typedef enum ray_settings
{
	RAY_SOLID	= 0x01,
	RAY_LIQUID	= 0x02,
	RAY_AIR		= 0x04,
} ray_settings_t;

/* Raycasts from start pointing in "direction" with maximum length "len." Returns the closest object that fits in the "settings" query bitmask. */
ray_t world_ray_cast(vector3_t start, vector3_t direction, float len, ray_settings_t settings);
/* Returns the block coords of the neighbor of a ray */
block_coords_t world_ray_neighbor(ray_t ray);

/* Loops through region and calls "callback" on each block */
void world_region_loop(block_coords_t min, block_coords_t max, world_loop_callback_t callback, void* user);
/* Sets contents of arr to a region's blocks -> AABBs. Returns the amount of blocks in the region. Writes to arr until arr no longer has space. */
int world_region_aabb(block_coords_t min, block_coords_t max, aabb_t* arr, int arr_len);

/* Gets block at coordinates */
block_type_t world_block_get(block_coords_t coords);
/* Sets block at coords to type */
void world_block_set(block_coords_t coords, block_type_t type);
/* Debugs info about a block to stream */
void world_block_debug(block_coords_t coords, FILE* stream);
/* Updates block */
void world_block_update(block_coords_t coords);
/* Stringifys block type */
const char* world_block_stringify(block_type_t type);

/* Updates world */
void world_update(float delta);
/* Renders world to screen */
void world_render(const shader_t solid, const shader_t liquid, float delta);

/* Gets world seed */
unsigned int world_seed(void);

#ifdef WORLD_INTERNAL

#include "graphics.h"

#define OPAQUE_BIT	1
#define LIQUID_BIT	2

struct chunk
{
	int x, z; /* The x and z coordinates in block space. As in, these numbers are multiples of 16 (chunk width and depth.) */
	int dirty_mask;
	vertex_buffer_t opaque_buffer, liquid_buffer;
	block_type_t arr[CHUNK_BLOCK_COUNT];
	bool generating; /* Is the chunk currently being generated? */
};

extern array_list_t chunk_list;		/* struct chunk array_list */

#define CHUNK_INDEX_OF(x, y, z)	((y) * CHUNK_FLOOR_BLOCK_COUNT + (z) * CHUNK_WX + (x))
#define CHUNK_AT(c, x, y, z)	((c)[CHUNK_INDEX_OF(x, y, z)])
#define CHUNK_X(mask)			((mask) % CHUNK_WX)
#define CHUNK_Y(mask)			((mask) / CHUNK_FLOOR_BLOCK_COUNT)
#define CHUNK_Z(mask)			((mask % CHUNK_FLOOR_BLOCK_COUNT) / CHUNK_WX)
#define CHUNK_FX(mask)			(float)((mask) % CHUNK_WX)
#define CHUNK_FY(mask)			(float)((mask) / CHUNK_FLOOR_BLOCK_COUNT)
#define CHUNK_FZ(mask)			(float)(((mask) % CHUNK_FLOOR_BLOCK_COUNT) / CHUNK_WX)

#define RADIUS 6
#define RADIUS_BLOCKS (RADIUS * 16)

void world_render_init(void);
void world_render_destroy(void);

/* Initializes world's chunk manager */
void world_chunk_init(unsigned int seed);
/* Destroys chunk manager */
void world_chunk_destroy(void);
/* Updates chunk manager */
void world_chunk_update(void);

/* Creates chunk at (x, z). Rounds down to a multiple to 16 (ex. 14 -> 0, -5 -> -16) */
struct chunk* world_chunk_create(int x_o, int z_o);
/* Adds chunk to list */
struct chunk* world_chunk_add(int x, int z, block_type_t chunk[CHUNK_BLOCK_COUNT]);
/* Removes chunk at position */
void world_chunk_remove(int x, int z);
/* Gets chunk containing block at (x, z). Returns NULL if it does not exist. */
struct chunk* world_chunk_get(int x, int z);
/* Cleans chunk's mesh */
void world_chunk_clean_mesh(struct chunk* chunk);

/* Saves chunk to file */
void world_file_load_world(unsigned int fallback_seed);
/* Saves chunk to file */
void world_file_save_chunk(int x, int z);
/* Saves current loaded chunks and player position to file */
void world_file_save_current(void);
/* Deletes current world file */
void world_file_delete(void);
/* Loads chunk from file. If it doesn't exist, returns NULL */
struct chunk* world_file_find_chunk(int x, int z);

#endif