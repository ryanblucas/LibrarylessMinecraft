/*
	graphics.c ~ RL
*/

#include "graphics.h"
#include <assert.h>
#include "camera.h"
#include "opengl.h"
#include <stdio.h>
#include "util.h"
#include <varargs.h>
#include "window.h"

#define ASSERT_NO_ERROR()	do { int err = glGetError(); if (err != GL_NO_ERROR) { printf("OpenGL error: %i\n", err); assert(false); } } while (false)

/* Any resource loading function aborts on failure because every resource is important. */

struct shader
{
	GLuint id;
	int uniform_count;
	struct uniform_stats
	{
		hash_t name_hash;	/* Hashed with mc_hash for storing more with less */
		GLint location;
		GLenum type;
	} uniform_map[];
};

struct vertex_buffer
{
	GLuint vao, vbo;
	GLsizei size, reserved;
	vertex_type_t type;
};

static shader_t current_shader;
static vertex_buffer_t current_buffer;
static sampler_t current_sampler;

static shader_t line_shader;
static struct vertex_buffer debug_buffer;
static array_list_t user_debug_buffers;

struct debug_primitive
{
	int start, size;
	double timestamp;
	hash_t hash;
};
static array_list_t primitives;

void graphics_init(void)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	line_shader = graphics_shader_load("assets/shaders/line_vertex.glsl", "assets/shaders/line_fragment.glsl");

	glGenVertexArrays(1, &debug_buffer.vao);
	glGenBuffers(1, &debug_buffer.vbo);
	debug_buffer.reserved = 64 * 24;
	debug_buffer.type = STANDARD_VERTEX;
	primitives = mc_list_create(sizeof(struct debug_primitive));

	glBindVertexArray(debug_buffer.vao);
	glBindBuffer(GL_ARRAY_BUFFER, debug_buffer.vbo);
	current_buffer = NULL;

	glBufferData(GL_ARRAY_BUFFER, debug_buffer.reserved * sizeof(float) * 3, NULL, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	ASSERT_NO_ERROR();

	user_debug_buffers = mc_list_create(sizeof(vertex_buffer_t));
}

void graphics_destroy(void)
{
	mc_list_destroy(&primitives);
	mc_list_destroy(&user_debug_buffers);
	graphics_shader_delete(&line_shader);
	glDeleteBuffers(1, &debug_buffer.vbo);
	glDeleteVertexArrays(1, &debug_buffer.vao);
	ASSERT_NO_ERROR();
}

void graphics_clear(color_t color)
{
	glClearColor((GLfloat)COLOR_RED(color) / 0xFF, (GLfloat)COLOR_GREEN(color) / 0xFF, (GLfloat)COLOR_BLUE(color) / 0xFF, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ASSERT_NO_ERROR();
}

sampler_t graphics_sampler_load(const char* path)
{
	/*	Bitmaps are saved LE, and the computer is assumed to match that. Only reason
		this is TO DO and not done already is because computers are commonly LE anyway */

	enum offset
	{
		OF_MAGIC = 0x00,
		OF_FILE_SIZE = 0x02,
		OF_OFFSET_DEFINITION = 0x0A,
		OF_INFO_HEADER = 0x0E,
	};

	long file_size;
	char* file = mc_read_file_binary(path, &file_size);
	mc_panic_if(memcmp(file + OF_MAGIC, "BM", 2) != 0, "bitmap header missing");
	mc_panic_if(*(int32_t*)(file + OF_FILE_SIZE) != file_size, "bitmap file size is invalid");
	int32_t pixel_data_offset = *(int32_t*)(file + OF_OFFSET_DEFINITION);

	int32_t width, height;
	GLenum format;

	uint32_t size = *(uint32_t*)(file + OF_INFO_HEADER);
	if (size == sizeof(BITMAPINFOHEADER)) /* version 3 */
	{
		printf("Loading a V3 DIB bitmap at path \"%s\"\n", path);

		BITMAPINFOHEADER bih;
		memcpy(&bih, file + OF_INFO_HEADER, sizeof bih);
		mc_panic_if(bih.biPlanes != 1, "bitmap planes is not 1");
		mc_panic_if(bih.biBitCount != 24 && bih.biBitCount != 32, "bitmap bit count unknown");
		mc_panic_if(bih.biCompression != BI_RGB, "bitmap compression unknown");
		mc_panic_if(bih.biClrImportant != 0, "bitmap important colors must be 0");

		width = bih.biWidth;
		height = bih.biHeight;
		format = bih.biBitCount == 24 ? GL_BGR : GL_BGRA;
	}
	else /* version 5, paint.NET saves in this format default */
	{
		mc_panic_if(size != sizeof(BITMAPV5HEADER), "bitmap is of an unknown version");
		printf("Loading a V5 DIB bitmap at path \"%s\"\n", path);

		BITMAPV5HEADER bih;
		memcpy(&bih, file + OF_INFO_HEADER, sizeof bih);
		mc_panic_if(bih.bV5Planes != 1, "bitmap planes is not 1");
		mc_panic_if(bih.bV5BitCount != 24 && bih.bV5BitCount != 32, "bitmap bit count unknown");
		mc_panic_if(bih.bV5Compression != BI_RGB && bih.bV5Compression != BI_BITFIELDS, "bitmap compression unknown");
		mc_panic_if(bih.bV5Compression == BI_BITFIELDS && bih.bV5BitCount != 32, "bitmap bit count must be 32 if compression is bitfields");
		mc_panic_if(bih.bV5ClrImportant != 0, "bitmap important colors must be 0");

		width = bih.bV5Width;
		height = bih.bV5Height;
		format = bih.bV5BitCount == 24 ? GL_BGR : GL_BGRA;
		if (bih.bV5Compression == BI_BITFIELDS)
		{
			mc_panic_if(bih.bV5AlphaMask != 0xFF000000
				|| bih.bV5RedMask != 0xFF0000
				|| bih.bV5GreenMask != 0xFF00
				|| bih.bV5BlueMask != 0xFF, "bitmap color masks must be 0xFF000000 (alpha), 0xFF00000 (blue), 0xFF00 (green), 0xFF (red)");
		}
	}

	GLuint res;
	glGenTextures(1, &res);
	glBindTexture(GL_TEXTURE_2D, res);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, file + pixel_data_offset);
	glGenerateMipmap(GL_TEXTURE_2D);

	free(file);

	ASSERT_NO_ERROR();
	return res;
}

void graphics_sampler_delete(sampler_t* sampler)
{
	glDeleteTextures(1, sampler);
	*sampler = 0;
	ASSERT_NO_ERROR();
}

void graphics_sampler_use(sampler_t handle)
{
	if (current_sampler == handle)
	{
		return;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, handle);
	ASSERT_NO_ERROR();
}

static GLuint graphics_create_shader_component(const char* path, GLenum type)
{
	GLuint shader = glCreateShader(type);
	const char* src = mc_read_file_text(path);
	mc_panic_if(!src, "component not found at path");

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char err[1024];
		glGetShaderInfoLog(shader, 1024, NULL, err);
		printf("%s\n", err);
		mc_panic_if(true, "component compile failure");
	}

	ASSERT_NO_ERROR();
	return shader;
}

static GLuint graphics_create_program(const char* vertex_path, const char* fragment_path)
{
	printf("Loading shader at \"%s\", \"%s\"\n", vertex_path, fragment_path);
	GLuint vert = graphics_create_shader_component(vertex_path, GL_VERTEX_SHADER),
		frag = graphics_create_shader_component(fragment_path, GL_FRAGMENT_SHADER);

	GLuint id = glCreateProgram();
	glAttachShader(id, vert);
	glAttachShader(id, frag);
	glLinkProgram(id);
	/*	From the Khronos Registry: "...deletion will not occur until glDetachShader is
		called to detach it from all program objects to which it is attached."
		The shader components must be detached to delete them. */
	glDetachShader(id, vert);
	glDeleteShader(vert);
	glDetachShader(id, frag);
	glDeleteShader(frag);

	GLint success;
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	mc_panic_if(!success, "program link failure");

	ASSERT_NO_ERROR();
	return id;
}

shader_t graphics_shader_load(const char* vertex_path, const char* fragment_path)
{
	GLuint id = graphics_create_program(vertex_path, fragment_path);
	GLint uniform_count;
	glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &uniform_count);
	struct shader* result = mc_malloc(sizeof * result + sizeof * result->uniform_map * uniform_count);
	result->id = id;
	result->uniform_count = uniform_count;

	for (int i = 0; i < uniform_count; i++)
	{
		GLint temp;
		char name[128];
		glGetActiveUniform(id, i, sizeof name, NULL, &temp, &result->uniform_map[i].type, name);
		result->uniform_map[i].name_hash = mc_hash(name, -1);
		result->uniform_map[i].location = glGetUniformLocation(id, name);
	}

	ASSERT_NO_ERROR();
	return result;
}

void graphics_shader_delete(shader_t* shader)
{
	if (*shader == current_shader)
	{
		glUseProgram(0);
		current_shader = NULL;
	}

	glDeleteProgram((*shader)->id);
	free(*shader);
	*shader = NULL;
	ASSERT_NO_ERROR();
}

static inline struct uniform_stats* graphics_shader_get_uniform(const char* name)
{
	hash_t name_hash = mc_hash(name, -1);
	for (int i = 0; i < current_shader->uniform_count; i++)
	{
		if (current_shader->uniform_map[i].name_hash == name_hash)
		{
			return &current_shader->uniform_map[i];
		}
	}
	return NULL;
}

void graphics_shader_use(shader_t shader)
{
	if (current_shader != shader)
	{
		current_shader = shader;
		glUseProgram(current_shader->id);
		ASSERT_NO_ERROR();
	}
}

void graphics_shader_matrix(const char* name, const matrix_t mat4)
{
	struct uniform_stats* curr = graphics_shader_get_uniform(name);
	mc_panic_if(!curr || curr->type != GL_FLOAT_MAT4, "shader missing a uniform mat4");
	glUniformMatrix4fv(curr->location, 1, GL_FALSE, mat4);
	ASSERT_NO_ERROR();
}

static inline void graphics_buffer_bind(struct vertex_buffer* buf)
{
	if (buf == current_buffer)
	{
		return;
	}
	current_buffer = buf;
	glBindVertexArray(buf->vao);
	glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
	ASSERT_NO_ERROR();
}

vertex_buffer_t graphics_buffer_create(const void* start, int len, vertex_type_t type)
{
	struct vertex_buffer* result = mc_malloc(sizeof * result);
	glGenVertexArrays(1, &result->vao);
	glGenBuffers(1, &result->vbo);
	result->size = result->reserved = (GLsizei)len;
	result->type = type;

	graphics_buffer_bind(result);

	if (type == BLOCK_VERTEX)
	{
		glBufferData(GL_ARRAY_BUFFER, len * sizeof(block_vertex_t), start, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(block_vertex_t), (void*)0);
	}
	else if (type == STANDARD_VERTEX)
	{
		glBufferData(GL_ARRAY_BUFFER, len * sizeof(vertex_t), start, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	}
	else
	{
		assert(false);
	}

	ASSERT_NO_ERROR();
	return result;
}

void graphics_buffer_modify(vertex_buffer_t buffer, const void* buf, size_t len)
{
	size_t element_size = buffer->type == BLOCK_VERTEX ? sizeof(block_vertex_t) : (sizeof(float) * 3);
	graphics_buffer_bind(buffer);
	if (len > buffer->reserved)
	{
		glBufferData(GL_ARRAY_BUFFER, len * element_size, buf, GL_STATIC_DRAW);
		buffer->size = buffer->reserved = (GLsizei)len;
		ASSERT_NO_ERROR();
		return;
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, len * element_size, buf);
	buffer->size = (GLsizei)len;
	ASSERT_NO_ERROR();
}

void graphics_buffer_delete(vertex_buffer_t* buffer)
{
	if (*buffer == NULL)
	{
		return;
	}

	if (*buffer == current_buffer)
	{
		current_buffer = NULL;
	}

	glDeleteBuffers(1, &(*buffer)->vbo);
	glDeleteVertexArrays(1, &(*buffer)->vao);
	free(*buffer);
	*buffer = NULL;
	ASSERT_NO_ERROR();
}

void graphics_buffer_draw(vertex_buffer_t buffer)
{
	graphics_buffer_bind(buffer);
	glDrawArrays(GL_TRIANGLES, 0, buffer->size);
	ASSERT_NO_ERROR();
}

void graphics_debug_clear(void)
{
	mc_list_splice(primitives, 0, mc_list_count(primitives));
}

static inline int graphics_debug_add_primitive(int length, hash_t hash)
{
	for (int i = 0; i < mc_list_count(primitives); i++)
	{
		struct debug_primitive* curr = MC_LIST_CAST_GET(primitives, i, struct debug_primitive);
		if (curr->hash == hash)
		{
			curr->timestamp = window_time();
			return curr->start;
		}
	}

	int final_end = 0;
	if (mc_list_count(primitives) > 0)
	{
		struct debug_primitive* final = MC_LIST_CAST_GET(primitives, mc_list_count(primitives) - 1, struct debug_primitive);
		final_end = final->start + final->size;
	}
	struct debug_primitive next =
	{
		.start = final_end,
		.size = length,
		.timestamp = window_time(),
		.hash = hash
	};
	mc_list_add(primitives, mc_list_count(primitives), &next, sizeof next);

	if (final_end >= debug_buffer.reserved)
	{
		graphics_buffer_bind(&debug_buffer);
		int old_reserved = debug_buffer.reserved * sizeof(float);
		float* data = mc_malloc(old_reserved);
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, old_reserved, data);

		debug_buffer.reserved *= 2;
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * debug_buffer.reserved, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, old_reserved, data);
		ASSERT_NO_ERROR();

		free(data);
	}
	return next.start;
}

void graphics_debug_set_line(vector3_t begin, vector3_t end)
{
	float pts[] = { begin.x, begin.y, begin.z, end.x, end.y, end.z };
	int buffer_pos = graphics_debug_add_primitive(2, mc_hash(pts, sizeof pts));
	graphics_buffer_bind(&debug_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, buffer_pos * sizeof(float) * 3, sizeof pts, pts);
	ASSERT_NO_ERROR();
}

void graphics_primitive_cube(vector3_t pos, vector3_t dim, vertex_t out[24])
{
	float pts[] =
	{
		pos.x, pos.y, pos.z,
		pos.x + dim.x, pos.y, pos.z,
		pos.x + dim.x, pos.y, pos.z,
		pos.x + dim.x, pos.y + dim.y, pos.z,
		pos.x + dim.x, pos.y + dim.y, pos.z,
		pos.x, pos.y + dim.y, pos.z,
		pos.x, pos.y + dim.y, pos.z,
		pos.x, pos.y, pos.z,
		pos.x, pos.y, pos.z + dim.z,
		pos.x + dim.x, pos.y, pos.z + dim.z,
		pos.x + dim.x, pos.y, pos.z + dim.z,
		pos.x + dim.x, pos.y + dim.y, pos.z + dim.z,
		pos.x + dim.x, pos.y + dim.y, pos.z + dim.z,
		pos.x, pos.y + dim.y, pos.z + dim.z,
		pos.x, pos.y + dim.y, pos.z + dim.z,
		pos.x, pos.y, pos.z + dim.z,
		pos.x, pos.y, pos.z,
		pos.x, pos.y, pos.z + dim.z,
		pos.x + dim.x, pos.y, pos.z,
		pos.x + dim.x, pos.y, pos.z + dim.z,
		pos.x, pos.y + dim.y, pos.z,
		pos.x, pos.y + dim.y, pos.z + dim.z,
		pos.x + dim.x, pos.y + dim.y, pos.z,
		pos.x + dim.x, pos.y + dim.y, pos.z + dim.z,
	};

	memcpy(out, pts, sizeof pts);
}

void graphics_debug_set_cube(vector3_t pos, vector3_t dim)
{
	float pts[72];
	graphics_primitive_cube(pos, dim, (vertex_t*)pts);
	int buffer_pos = graphics_debug_add_primitive(24, mc_hash(pts, sizeof pts));
	graphics_buffer_bind(&debug_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, buffer_pos * sizeof(float) * 3, sizeof pts, pts);
	ASSERT_NO_ERROR();
}

void graphics_debug_draw(void)
{
	if (window_input_clicked(INPUT_TOGGLE_WIREFRAME))
	{
		static bool wireframe;
		wireframe = !wireframe;
		if (wireframe)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	graphics_buffer_bind(&debug_buffer);
	graphics_shader_use(line_shader);
	camera_activate();
	struct uniform_stats* us = graphics_shader_get_uniform("u_color");
	mc_panic_if(!us || us->type != GL_INT, "invalid debug shader");
	glUniform1i(us->location, COLOR_CREATE(255, 255, 255));

	if (mc_list_count(primitives) > 0)
	{
		struct debug_primitive* first = MC_LIST_CAST_GET(primitives, 0, struct debug_primitive),
			* last = MC_LIST_CAST_GET(primitives, mc_list_count(primitives) - 1, struct debug_primitive);
		int start = first->start, count = last->start + last->size - first->start;
		glDrawArrays(GL_LINES, start, count);
	}
	
	for (int i = 0; i < mc_list_count(user_debug_buffers); i++)
	{
		vertex_buffer_t curr = *MC_LIST_CAST_GET(user_debug_buffers, i, vertex_buffer_t);
		graphics_buffer_bind(curr);
		glDrawArrays(GL_LINES, 0, curr->size);
	}
	mc_list_splice(user_debug_buffers, 0, mc_list_count(user_debug_buffers));
	
	ASSERT_NO_ERROR();

	for (int i = mc_list_count(primitives) - 1; i >= 0; i--)
	{
		if (window_time() - MC_LIST_CAST_GET(primitives, i, struct debug_primitive)->timestamp > 3.5F)
		{
			mc_list_remove(primitives, i, NULL, 0);
		}
	}
}

void graphics_debug_draw_buffer(vertex_buffer_t buffer)
{
	mc_list_add(user_debug_buffers, mc_list_count(user_debug_buffers), &buffer, sizeof buffer);
}