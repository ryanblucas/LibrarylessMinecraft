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

typedef int sampler_t;
typedef struct shader* shader_t;

typedef struct vertex_buffer* vertex_buffer_t;
#pragma pack(push, 1)
/* Raw vertex data, sent straight to the GPU */
typedef struct vertex
{
	float x, y, z;
	float tx, ty;
} vertex_t;
#pragma pack(pop)

/* Initializes graphics objects and state */
void graphics_init(void);
/* Deletes graphics objects, does not reset state! */
void graphics_destroy(void);

/* Clears color and depth buffers. Clears color to color passed */
void graphics_clear(color_t color);

/* Loads a DIB bitmap V3/V5 at path into VRAM and returns handle. */
sampler_t graphics_sampler_load(const char* path);
/* Deletes sampler and sets handle to 0. */
void graphics_sampler_delete(sampler_t* sampler);
/* Sets a sampler to use in a unit. */
void graphics_sampler_use(sampler_t handle);

/*	Loads shader given vertex shader path/directory and fragment shader path/directory. 
	Both must be for OpenGL 4.6. Returns 0 on failure, or a handle to the shader object. */
shader_t graphics_shader_load(const char* vertex_path, const char* fragment_path);
/* Deletes the shader object and sets the pointer to NULL */
void graphics_shader_delete(shader_t* shader);
/* Sets current shader. */
void graphics_shader_use(shader_t shader);
/* Sets a shader's model transform uniform */
void graphics_shader_matrix(const char* name, const matrix_t mat4);

/*	Creates vertex buffer. Start and len can both be 0, but if 
	they aren't they specify the starting values for the buffer. */
vertex_buffer_t graphics_buffer_create(const vertex_t* start, size_t len);
/* Modifies existing vertex buffer. */
void graphics_buffer_modify(vertex_buffer_t buffer, const vertex_t* buf, size_t len);
/* Deletes the vertex buffer and sets the pointer to NULL */
void graphics_buffer_delete(vertex_buffer_t* buffer);
/* Draws vertex buffer with current shader. */
void graphics_buffer_draw(vertex_buffer_t buffer);

#define GRAPHICS_DEBUG_SET_BLOCK(coords) graphics_debug_set_cube(block_coords_to_vector(coords), (vector3_t) { 1.0F, 1.0F, 1.0F })
#define GRAPHICS_DEBUG_SET_AABB(aabb) graphics_debug_set_cube((aabb).min, aabb_dimensions(aabb))

/* Clears debug buffer */
void graphics_debug_clear(void);
/* Sets line to draw from "begin" to "end" */
void graphics_debug_set_line(vector3_t begin, vector3_t end);
/* Sets cube to draw at "pos" with dimensions "dim" */
void graphics_debug_set_cube(vector3_t pos, vector3_t dim);
/* Draws debug buffer */
void graphics_debug_draw(void);