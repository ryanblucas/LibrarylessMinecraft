/*
	graphics.h ~ RL
*/

#pragma once

#include <stdint.h>
#include "util.h"

#define COLOR_CREATE(r, g, b)	(((r) & 0xFF) | (((g) & 0xFF) << 8) | (((b) & 0xFF) << 16))
#define COLOR_RED(packed)		((packed) & 0xFF)
#define COLOR_GREEN(packed)		(((packed) >> 8) & 0xFF)
#define COLOR_BLUE(packed)		(((packed) >> 16) & 0xFF)

typedef uint32_t color_t;

typedef struct sampler* sampler_t;
typedef struct shader* shader_t;

typedef struct vertex_buffer* vertex_buffer_t;
/* Raw block/chunk vertex data, sent straight to the GPU. First 10 bits are 5-bit position (XZ), Y is next at 9-bit,
	next 2 bits are X and Y for texture coordinates and the next 8 bits designate which texture ID to use. */
typedef uint32_t block_vertex_t;

typedef struct vertex
{
	float x, y, z;
	float tx, ty;
} vertex_t;

typedef struct debug_vertex
{
	float x, y, z;
	color_t color;
} debug_vertex_t;

typedef struct interface_vertex
{
	float x, y, z;
	float tx, ty;
	color_t color;
} interface_vertex_t;

typedef enum vertex_type
{
	VERTEX_BLOCK,		/* Expects array of block_vertex_t */
	VERTEX_STANDARD,	/* Expects array of vertex_t */
	VERTEX_POSITION,	/* Expects array of floats, three making up one position */
	VERTEX_DEBUG,		/* Expects array of debug_vertex_t */
	VERTEX_INTERFACE	/* Expects array of interface_vertex_t */
} vertex_type_t;

#define CREATE_BLOCK_VERTEX_POS(x, y, z)			((x) | ((z) << 5) | ((y) << 10))
#define SET_BLOCK_VERTEX_TEXTURE(v, tx, ty, tid, i)	((v) | ((tx) << 19) | ((ty) << 20) | ((tid) << 21) | ((i) << 29))

/* Initializes graphics objects and state */
void graphics_init(void);
/* Deletes graphics objects, does not reset state! */
void graphics_destroy(void);

/* Clears color and depth buffers. Clears color to color passed */
void graphics_clear(color_t color);
/* Clears depth buffer only */
void graphics_clear_depth(void);

/* Loads a DIB bitmap V3/V5 at path into VRAM and returns handle. */
sampler_t graphics_sampler_load(const char* path);
/* Deletes sampler and sets handle to 0. */
void graphics_sampler_delete(sampler_t* sampler);
/* Sets a sampler to use. */
void graphics_sampler_use(sampler_t handle);
/* queue width of sampler */
int graphics_sampler_width(sampler_t sampler);
/* queue height of sampler */
int graphics_sampler_height(sampler_t sampler);

/*	Loads shader given vertex shader path/directory and fragment shader path/directory. 
	Both must be for OpenGL 4.6. Returns 0 on failure, or a handle to the shader object. */
shader_t graphics_shader_load(const char* vertex_path, const char* fragment_path);
/* Deletes the shader object and sets the pointer to NULL */
void graphics_shader_delete(shader_t* shader);
/* Sets current shader. */
void graphics_shader_use(shader_t shader);
/* Sets a shader's model uniform */
void graphics_shader_matrix(const char* name, const matrix_t mat4);
/* Sets a shader's int uniform */
void graphics_shader_int(const char* name, int i);

/*	Creates vertex buffer. Start and len can both be 0, but if 
	they aren't they specify the starting values for the buffer.
	len refers to the count of elements, not the count of bytes. */
vertex_buffer_t graphics_buffer_create(const void* start, int len, vertex_type_t type);
/* Modifies existing vertex buffer. */
void graphics_buffer_modify(vertex_buffer_t buffer, const void* buf, int len);
/* Deletes the vertex buffer and sets the pointer to NULL */
void graphics_buffer_delete(vertex_buffer_t* buffer);
/* Draws vertex buffer w/ GL_TRIANGLES with current shader. */
void graphics_buffer_draw(vertex_buffer_t buffer);

#define GRAPHICS_DEBUG_SET_BLOCK(coords) graphics_debug_set_cube(block_coords_to_vector(coords), (vector3_t) { 1.0F, 1.0F, 1.0F })
#define GRAPHICS_DEBUG_SET_AABB(aabb) graphics_debug_set_cube((aabb).min, aabb_dimensions(aabb))

typedef struct debug_buffer
{
	vertex_buffer_t vertex;	/* Either of type "POSITION_VERTEX," where it renders lines white, or "DEBUG_VERTEX" */
	vector3_t position;		/* Offset to render vertex buffer */
} debug_buffer_t;

/* Clears debug buffer */
void graphics_debug_clear(void);
/* Sets line to draw from "begin" to "end" */
void graphics_debug_set_line(vector3_t begin, vector3_t end);
/* Calculates a cube given its position and dimensions and stores in out */
void graphics_primitive_cube(vector3_t pos, vector3_t dim, float out[72]);
/* Sets cube to draw at "pos" with dimensions "dim" */
void graphics_debug_set_cube(vector3_t pos, vector3_t dim);
/* Draws debug buffer */
void graphics_debug_draw(void);
/* Draws a vertex buffer using the debug shader. This does not immediately draw the buffer,
	it instead waits till graphics_debug_draw is called to preserve any active shader state.
	This vertex buffer is drawn using GL_LINES as opposed to graphics_buffer_draw's GL_TRIANGLES. */
void graphics_debug_queue_buffer(debug_buffer_t buffer);