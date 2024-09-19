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

#define IS_INVALID_BLOCK_COORDS(bc) ((bc).y < 0 || (bc).y >= CHUNK_WY)

enum
{
	BLOCK_AIR,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_STONE,
	BLOCK_WATER,

	BLOCK_COUNT
};
typedef uint8_t block_type_t;

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

void world_init(void);
void world_destroy(void);

/* Raycasts from start pointing in "direction" with maximum length "len." */
ray_t world_ray_cast(vector3_t start, vector3_t direction, float len);
/* Returns the block coords of the neighbor of a ray */
block_coords_t world_ray_neighbor(ray_t ray);

/* Loops through region and calls "callback" on each block */
void world_region_loop(block_coords_t min, block_coords_t max, world_loop_callback_t callback, void* user);
/* Sets contents of arr to a region's blocks -> AABBs. Returns the amount of blocks in the region. Writes to arr until arr no longer has space. */
int world_region_aabb(block_coords_t min, block_coords_t max, aabb_t* arr, size_t arr_len);

/* Gets block at coordinates */
block_type_t world_block_get(block_coords_t coords);
/* Sets block at coords to type */
void world_block_set(block_coords_t coords, block_type_t type);

/* Updates world */
void world_update(float delta);
/* Renders world to screen */
void world_render(float delta);

#ifdef WORLD_INTERNAL

#include "graphics.h"

#define CHUNK_INDEX_OF(x, y, z)	((y) * CHUNK_FLOOR_BLOCK_COUNT + (z) * (CHUNK_WX) + (x))
#define CHUNK_AT(c, x, y, z)	((c)[CHUNK_INDEX_OF(x, y, z)])
#define CHUNK_X(mask)			((mask) % CHUNK_WX)
#define CHUNK_Y(mask)			((mask) / CHUNK_FLOOR_BLOCK_COUNT)
#define CHUNK_Z(mask)			((mask % CHUNK_FLOOR_BLOCK_COUNT) / CHUNK_WX)
#define CHUNK_FX(mask)			(float)((mask) % CHUNK_WX)
#define CHUNK_FY(mask)			(float)((mask) / CHUNK_FLOOR_BLOCK_COUNT)
#define CHUNK_FZ(mask)			(float)((mask % CHUNK_FLOOR_BLOCK_COUNT) / CHUNK_WX)

void world_chunk_mesh(vertex_buffer_t out, const block_type_t chunk[CHUNK_BLOCK_COUNT]);

#endif