/*
	world.h ~ RL
*/

#pragma once

#include "entity.h"
#include "util.h"

#define CHUNK_WX 16
#define CHUNK_WY 256
#define CHUNK_WZ 16
#define CHUNK_FLOOR_BLOCK_COUNT	(CHUNK_WX * CHUNK_WZ)
#define CHUNK_BLOCK_COUNT		(CHUNK_WX * CHUNK_WY * CHUNK_WZ)

#define WATER_STRENGTH 7

#define IS_INVALID_BLOCK_COORDS(bc) ((bc).y < 0 || (bc).y >= CHUNK_WY)
#define IS_SOLID(type) ((type) != BLOCK_AIR && (type) != BLOCK_WATER)

enum
{
#define DEFINE_BLOCK(name) BLOCK_ ## name,
#define BLOCK_LIST \
	DEFINE_BLOCK(AIR) \
	DEFINE_BLOCK(GRASS) \
	DEFINE_BLOCK(DIRT) \
	DEFINE_BLOCK(STONE) \
	DEFINE_BLOCK(WATER) \

	BLOCK_LIST
	BLOCK_COUNT
#undef DEFINE_BLOCK
};
typedef uint8_t block_type_t;

typedef struct block_coords
{
	int x, y, z;
} block_coords_t;

typedef struct liquid
{
	block_coords_t position; /* Position of liquid */
	vector3_t push;			/* Which way the liquid pushes */
	int strength;			/* Strength of liquid (1-7) */
	int tick_count;			/* When the liquid was last updated */
	bool came_from_above;	/* Whether or not the liquid came from above */
} liquid_t;

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
int world_region_aabb(block_coords_t min, block_coords_t max, aabb_t* arr, size_t arr_len);

/* Converts a liquid to an AABB */
aabb_t world_liquid_to_aabb(liquid_t liquid);
/* Gets liquid at position */
liquid_t* world_liquid_get(block_coords_t coords);
/* Adds liquid to position with settings. Will update at coords, so this will spread. */
liquid_t* world_liquid_add(block_coords_t coords, vector3_t push, int strength);
/* Removes all liquids at position */
void world_liquid_remove(block_coords_t coords);
/* Adds the liquid average of what's around it at coords */
void world_liquid_spread(block_coords_t coords);

/* Gets block at coordinates */
block_type_t world_block_get(block_coords_t coords);
/* Sets block at coords to type */
void world_block_set(block_coords_t coords, block_type_t type);
/* Debugs info about a block to stream */
void world_block_debug(block_coords_t coords, FILE* stream);
/* Updates block */
void world_block_update(block_coords_t coords);

/* Updates world */
void world_update(float delta);
/* Renders world to screen */
void world_render(float delta);

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
	array_list_t flowing_liquid; /* struct liquid array_list */
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

/* Initializes world's chunk manager */
void world_chunk_init(void);
/* Destroys chunk manager */
void world_chunk_destroy(void);
/* Creates chunk at (x, z). Rounds down to a multiple to 16 (ex. 14 -> 0, -5 -> -16) */
struct chunk* world_chunk_create(int x_o, int z_o);
/* Gets chunk containing block at (x, z). Returns NULL if it does not exist. */
struct chunk* world_chunk_get(int x, int z);
/* Cleans chunk at index in chunk list. */
void world_chunk_clean_mesh(int index);

#endif