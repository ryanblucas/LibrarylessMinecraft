/*
	world_render.c ~ RL
	Generates the mesh for a chunk and renders it
*/

#define WORLD_INTERNAL
#include "world.h"
#include <assert.h>
#include "camera.h"
#include "window.h"

static vertex_buffer_t debug_chunk_border;
static bool display_debug_chunk_border;

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

static void world_mesh_quad(int mask, int type, enum quad_normal normal)
{
	static const faces[BLOCK_COUNT - 1][8] =
	{
		/* { 0, RIGHT, DOWN, BACKWARD, 0, LEFT, UP, FORWARD } */

		{ 0, 1, 0, 1, 0, 1, 2, 1 }, /* BLOCK_GRASS */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }, /* BLOCK_DIRT */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }, /* BLOCK_STONE */
		{ 0, 1, 0, 1, 0, 1, 1, 1 }, /* BLOCK_WATER */
		{ 0, 0, 1, 0, 0, 0, 1, 0 }, /* BLOCK_LOG */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }, /* BLOCK_LEAVES */
	};
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
		c = SET_BLOCK_VERTEX_TEXTURE(c, 0, 0, type, faces[type][normal]);
		b = SET_BLOCK_VERTEX_TEXTURE(b, 1, 0, type, faces[type][normal]);
		a = SET_BLOCK_VERTEX_TEXTURE(a, 1, 1, type, faces[type][normal]);
		d = SET_BLOCK_VERTEX_TEXTURE(d, 0, 1, type, faces[type][normal]);
	}
	else
	{
		a = SET_BLOCK_VERTEX_TEXTURE(a, 1, 0, type, faces[type][normal]);
		b = SET_BLOCK_VERTEX_TEXTURE(b, 0, 0, type, faces[type][normal]);
		c = SET_BLOCK_VERTEX_TEXTURE(c, 0, 1, type, faces[type][normal]);
		d = SET_BLOCK_VERTEX_TEXTURE(d, 1, 1, type, faces[type][normal]);
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

static void world_chunk_clean(struct chunk* chunk)
{
	static const block_type_t def[CHUNK_BLOCK_COUNT];
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
		if (y == 0 || !IS_SOLID(CHUNK_AT(arr, x, y - 1, z)))
		{
			world_mesh_quad(mask, curr, UP);
		}
		if (y == CHUNK_WY - 1 || !IS_SOLID(CHUNK_AT(arr, x, y + 1, z)))
		{
			world_mesh_quad(mask, curr, DOWN);
		}

		if (x == 0)
		{
			if (!IS_SOLID(CHUNK_AT(left, CHUNK_WX - 1, y, z)))
			{
				world_mesh_quad(mask, curr, LEFT);
			}
		}
		else if (!IS_SOLID(CHUNK_AT(arr, x - 1, y, z)))
		{
			world_mesh_quad(mask, curr, LEFT);
		}
		if (x == CHUNK_WX - 1)
		{
			if (!IS_SOLID(CHUNK_AT(right, 0, y, z)))
			{
				world_mesh_quad(mask, curr, RIGHT);
			}
		}
		else if (!IS_SOLID(CHUNK_AT(arr, x + 1, y, z)))
		{
			world_mesh_quad(mask, curr, RIGHT);
		}
		if (z == 0)
		{
			if (!IS_SOLID(CHUNK_AT(backward, x, y, CHUNK_WZ - 1)))
			{
				world_mesh_quad(mask, curr, BACKWARD);
			}
		}
		else if (!IS_SOLID(CHUNK_AT(arr, x, y, z - 1)))
		{
			world_mesh_quad(mask, curr, BACKWARD);
		}
		if (z == CHUNK_WZ - 1)
		{
			if (!IS_SOLID(CHUNK_AT(forward, x, y, 0)))
			{
				world_mesh_quad(mask, curr, FORWARD);
			}
		}
		else if (!IS_SOLID(CHUNK_AT(arr, x, y, z + 1)))
		{
			world_mesh_quad(mask, curr, FORWARD);
		}
	}

	graphics_buffer_modify(chunk->opaque_buffer, block_vertex_list->array, block_vertex_list->count);
	block_vertex_list->count = 0;
	chunk->dirty_mask ^= OPAQUE_BIT;
}

static void world_chunk_clean_liquid(struct chunk* chunk)
{
	static const block_type_t def[CHUNK_BLOCK_COUNT];
	const block_type_t* left = world_chunk_get_array_or_default(chunk->x - CHUNK_WX, chunk->z, def),
		* right = world_chunk_get_array_or_default(chunk->x + CHUNK_WX, chunk->z, def),
		* backward = world_chunk_get_array_or_default(chunk->x, chunk->z - CHUNK_WZ, def),
		* forward = world_chunk_get_array_or_default(chunk->x, chunk->z + CHUNK_WZ, def);
	const block_type_t* arr = chunk->arr;

	for (int mask = 0; mask < CHUNK_BLOCK_COUNT; mask++)
	{
		int x = CHUNK_X(mask), y = CHUNK_Y(mask), z = CHUNK_Z(mask);
		block_type_t curr = chunk->arr[mask];
		if (curr != BLOCK_WATER)
		{
			continue;
		}
		curr--;
		if (y == 0 || CHUNK_AT(arr, x, y - 1, z) != BLOCK_WATER)
		{
			world_mesh_quad(mask, curr, UP);
		}
		if (y == CHUNK_WY - 1 || CHUNK_AT(arr, x, y + 1, z) != BLOCK_WATER)
		{
			world_mesh_quad(mask, curr, DOWN);
		}
		if (x == 0)
		{
			if (CHUNK_AT(left, CHUNK_WX - 1, y, z) != BLOCK_WATER)
			{
				world_mesh_quad(mask, curr, LEFT);
			}
		}
		else if (CHUNK_AT(arr, x - 1, y, z) != BLOCK_WATER)
		{
			world_mesh_quad(mask, curr, LEFT);
		}
		if (x == CHUNK_WX - 1)
		{
			if (CHUNK_AT(right, 0, y, z) != BLOCK_WATER)
			{
				world_mesh_quad(mask, curr, RIGHT);
			}
		}
		else if (CHUNK_AT(arr, x + 1, y, z) != BLOCK_WATER)
		{
			world_mesh_quad(mask, curr, RIGHT);
		}
		if (z == 0)
		{
			if (CHUNK_AT(backward, x, y, CHUNK_WZ - 1) != BLOCK_WATER)
			{
				world_mesh_quad(mask, curr, BACKWARD);
			}
		}
		else if (CHUNK_AT(arr, x, y, z - 1) != BLOCK_WATER)
		{
			world_mesh_quad(mask, curr, BACKWARD);
		}
		if (z == CHUNK_WZ - 1)
		{
			if (CHUNK_AT(forward, x, y, 0) != BLOCK_WATER)
			{
				world_mesh_quad(mask, curr, FORWARD);
			}
		}
		else if (CHUNK_AT(arr, x, y, z + 1) != BLOCK_WATER)
		{
			world_mesh_quad(mask, curr, FORWARD);
		}
	}

	graphics_buffer_modify(chunk->liquid_buffer, block_vertex_list->array, block_vertex_list->count);
	block_vertex_list->count = 0;
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

	struct chunk* chunk = MC_LIST_CAST_GET(chunk_list, index, struct chunk);
	if (chunk->dirty_mask & OPAQUE_BIT)
	{
		world_chunk_clean(chunk);
	}
	if (chunk->dirty_mask & LIQUID_BIT)
	{
		world_chunk_clean_liquid(chunk);
	}
}

void world_render_init(void)
{
	array_list_t vertices = mc_list_create(sizeof(float));
	for (int j = 0; j < CHUNK_WY; j += 16)
	{
		float to_add[72];
		graphics_primitive_cube((vector3_t) { 0, (float)j, 0 }, (vector3_t) { 16, 16, 16 }, to_add);
		mc_list_array_add(vertices, mc_list_count(vertices), to_add, sizeof * to_add, 72);
	}
	debug_chunk_border = graphics_buffer_create(mc_list_array(vertices), mc_list_count(vertices) / 3, VERTEX_POSITION);
	mc_list_destroy(&vertices);
}

void world_render_destroy(void)
{
	graphics_buffer_delete(&debug_chunk_border);
}

void world_render(const shader_t solid, const shader_t liquid, float delta)
{
	graphics_shader_use(solid);
	matrix_t cam;
	camera_view_projection(cam);
	graphics_shader_matrix("camera", cam);
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		struct chunk* chunk = MC_LIST_CAST_GET(chunk_list, i, struct chunk);
		world_chunk_clean_mesh(i);

		matrix_t transform;
		matrix_translation((vector3_t) { (float)chunk->x, 0.0F, (float)chunk->z }, transform);
		graphics_shader_matrix("model", transform);
		graphics_buffer_draw(chunk->opaque_buffer);
	}

	graphics_shader_use(liquid);
	graphics_shader_matrix("camera", cam);
	for (int i = 0; i < mc_list_count(chunk_list); i++)
	{
		struct chunk* chunk = MC_LIST_CAST_GET(chunk_list, i, struct chunk);
		matrix_t transform;
		matrix_translation((vector3_t) { (float)chunk->x, 0.0F, (float)chunk->z }, transform);
		graphics_shader_matrix("model", transform);
		graphics_buffer_draw(chunk->liquid_buffer);
	}

	static int prev_tick = -1;
	if (prev_tick != world_ticks())
	{
		prev_tick = world_ticks();
		if (window_input_clicked(INPUT_TOGGLE_CHUNK_BORDERS))
		{
			display_debug_chunk_border = !display_debug_chunk_border;
		}
	}

	if (display_debug_chunk_border)
	{
		vector3_t player_pos = aabb_get_center(player.hitbox);
		player_pos.x = ROUND_DOWN(round(player_pos.x), CHUNK_WX);
		player_pos.y = 0.0F;
		player_pos.z = ROUND_DOWN(round(player_pos.z), CHUNK_WZ);
		graphics_debug_queue_buffer((debug_buffer_t)
		{
			.vertex = debug_chunk_border,
				.position = player_pos
		});
	}
}