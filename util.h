/*
	util.h ~ RL
*/

#pragma once

/* TO DO: Split some of this stuff into different files! */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Implementation of an array list: an array that once its capacity--reserved--is reached, it reallocates and doubles its capacity. */
typedef struct array_list* array_list_t;

#define MC_LIST_CAST_GET(list, index, type) (assert(sizeof(type) == mc_list_element_size(list) && (index) >= 0 && (index) < mc_list_count(list)), (type*)mc_list_array(list) + (index))

/* Get amount of elements in list */
int mc_list_count(const array_list_t list);
/* Get size of each element in list */
size_t mc_list_element_size(const array_list_t list);
/* Get raw array of elements */
void* mc_list_array(array_list_t list);

/* Creates an array list with name and size of each element. */
array_list_t mc_list_create(size_t element_size);
/* Destroys list pointed at by parameter, setting parameter to NULL afterwards */
void mc_list_destroy(array_list_t* list);
/* Adds an element to the array list. You may reserve an element by passing NULL to pelement and write to it with the pointer from mc_list_array. Returns index that was added */
int mc_list_add(array_list_t list, int index, const void* pelement, size_t element_size);
/* Removes an element from the array list and writes to out if not NULL. Returns index that was deleted */
int mc_list_remove(array_list_t list, int index, void* out, size_t element_size);
/* Splices an array list */
void mc_list_splice(array_list_t list, int start, int count);

/* Cleans game state and crashes. *ONLY CALL IN EMERGENCY* */
extern inline void mc_panic_if(bool condition, const char* reason)
{
	if (!condition)
	{
		return;
	}
	printf("Crashing: %s\n", reason);
	exit(1);
}

/* Allocates at least size bytes. Use std free once done. Never returns NULL, if no memory is available then it calls "panic_callback." */
extern inline void* mc_malloc(size_t size)
{
	void* res = malloc(size);
	if (!res)
	{
		mc_panic_if(true, "malloc failed");
		return NULL;
	}
	return res;
}

/* Reads file at path, returns NULL-terminated pointer to full file contents. Allocates memory on the heap, std free once done. */
extern inline char* mc_read_file_text(const char* path)
{
	FILE* file = fopen(path, "r");
	if (!file)
	{
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* src = mc_malloc(size + 1);
	size_t real_end = fread(src, 1, size, file);
	fclose(file);

	src[real_end] = '\0';
	return src;
}

/*	Reads file at path, returns pointer to full file contents. There is no text formatting, unlike mc_read_file_text.
	Allocates memory on the heap, std free once done. */
extern inline uint8_t* mc_read_file_binary(const char* path, long* size)
{
	FILE* file = fopen(path, "rb");
	if (!file)
	{
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	fseek(file, 0, SEEK_SET);
	uint8_t* src = mc_malloc(*size);
	size_t amount_read = fread(src, 1, *size, file);
	fclose(file);

	if (amount_read != *size)
	{
		free(src);
		src = NULL;
	}
	return src;
}

typedef uintmax_t hash_t;

/* Hashes buffer. If buf_len < 0, buf is assumed to be NUL-terminated. */
extern inline hash_t mc_hash(const void* _buf, int buf_len)
{
	const uint8_t* buf = (uint8_t*)_buf;
	hash_t result = 5381;
	for (int i = 0; i < buf_len || (buf_len < 0 && buf[i]); i++)
	{
		result = ((result << 5) + result) ^ buf[i];
	}
	return result;
}

/* MATH SECTION */

#include <float.h>
#define _USE_MATH_DEFINES
#include <math.h>

/* Compares two floats for equality */
extern inline bool float_equals(float a, float b)
{
	float absA = fabsf(a);
	float absB = fabsf(b);
	float diff = fabsf(a - b);

	if (a == b)
	{
		return true;
	}
	else if (a == 0 || b == 0 || diff < FLT_MIN)
	{
		return diff < (FLT_EPSILON * FLT_MIN);
	}
	else
	{
		return diff / (absA + absB) < FLT_EPSILON;
	}
}

#include <intrin.h>
#include <string.h>

#define MATRIX_IDENTITY			((matrix_t){ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 })
#define VECTOR3_IDENTITY		((vector3_t){ 0, 0, 0 })
#define RADIANS_TO_DEGREES(a)	((a) * 180 / (float)M_PI)
#define DEGREES_TO_RADIANS(a)	((a) * (float)M_PI / 180)
#define INDEX_MATRIX(m, r, c)	((m)[(r) * 4 + (c)])

/* Row-major float array of 16 elements/4x4. */
typedef float matrix_t[16];
typedef struct vector3
{
	float x, y, z;
	float reserved; /* aligned with a 128-bit SIMD register */
} vector3_t;

typedef union vector3_uarray
{
	vector3_t vec;
	float raw[4];
} vector3_uarray_t;

/* TO DO: Scalar functions use _mm_set1_ps, which can get expanded into multiple instructions--is that really faster overall? */
/* TO DO: Vector functions are often chained together, which means the constant loading (_mm_load_ps) is redundant. */

/* adds two vectors together and returns the result */
extern inline vector3_t vector3_add(vector3_t a, vector3_t b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_add_ps(_a, _b));
	return res;
}

/* subtracts two vectors and returns the result */
extern inline vector3_t vector3_sub(vector3_t a, vector3_t b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_sub_ps(_a, _b));
	return res;
}

/* Multiplies two vectors together and returns the result */
extern inline vector3_t vector3_mul(vector3_t a, vector3_t b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_mul_ps(_a, _b));
	return res;
}

/* Divides two vectors and returns the result */
extern inline vector3_t vector3_div(vector3_t a, vector3_t b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_div_ps(_a, _b));
	return res;
}

/* Adds contents of a with scalar b */
extern inline vector3_t vector3_add_scalar(vector3_t a, float b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_set1_ps(b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_add_ps(_a, _b));
	return res;
}

/* Subtracts contents of a with scalar b */
extern inline vector3_t vector3_sub_scalar(vector3_t a, float b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_set1_ps(b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_sub_ps(_a, _b));
	return res;
}

/* Subtracts scalar b with contents of a -> (b - a.x, b - a.y, ...) */
extern inline vector3_t vector3_scalar_sub(float b, vector3_t a)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_set1_ps(b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_sub_ps(_b, _a));
	return res;
}

/* Multiplies contents of a with scalar b */
extern inline vector3_t vector3_mul_scalar(vector3_t a, float b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_set1_ps(b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_mul_ps(_a, _b));
	return res;
}

/* Divides contents of a with scalar b */
extern inline vector3_t vector3_div_scalar(vector3_t a, float b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_set1_ps(b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_div_ps(_a, _b));
	return res;
}

/* Divides scalar b with contents of a -> (b / a.x, b / a.y, ...) */
extern inline vector3_t vector3_scalar_div(float b, vector3_t a)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_set1_ps(b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_div_ps(_b, _a));
	return res;
}

/* Returns a vector where each component is the absolute value of the parameter's respective component */
extern inline vector3_t vector3_abs(vector3_t a)
{
	__m128 mask = _mm_set1_ps(-0.0F),
		_a = _mm_load_ps((float*)&a);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_andnot_ps(mask, _a));
	return res;
}

/* Returns a vector where each component is the lesser of the two parameters' respective components */
extern inline vector3_t vector3_min(vector3_t a, vector3_t b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_min_ps(_b, _a));
	return res;
}

/* Returns a vector where each component is the greater of the two parameters' respective components */
extern inline vector3_t vector3_max(vector3_t a, vector3_t b)
{
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_max_ps(_b, _a));
	return res;
}

extern inline float vector3_dot(vector3_t a, vector3_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

extern inline float vector3_magnitude(vector3_t a)
{
	return sqrtf(vector3_dot(a, a));
}

extern inline float vector3_distance(vector3_t a, vector3_t b)
{
	vector3_t c = vector3_sub(b, a);
	return sqrtf(c.x * c.x + c.y * c.y + c.z * c.z);
}

/* Normalizes vector a and returns result */
extern inline vector3_t vector3_normalize(vector3_t a)
{
	float mag = vector3_magnitude(a);
	/* Way to remove this branch? TO DO */
	if (float_equals(mag, 0.0F))
	{
		return a;
	}

	__m128 _a = _mm_load_ps((float*)&a),
		_mag = _mm_set1_ps(mag);

	vector3_t res;
	_mm_store_ps((float*)&res, _mm_div_ps(_a, _mag));
	return res;
}

/* Cross product between vector A and B */
extern inline vector3_t vector3_cross(vector3_t a, vector3_t b)
{
	/* https://geometrian.com/programming/tutorials/cross-product/index.php */
	__m128 _a = _mm_load_ps((float*)&a),
		_b = _mm_load_ps((float*)&b);
	__m128 tmp0 = _mm_shuffle_ps(_a, _a, _MM_SHUFFLE(3, 0, 2, 1)),
		tmp1 = _mm_shuffle_ps(_b, _b, _MM_SHUFFLE(3, 1, 0, 2)),
		tmp2 = _mm_mul_ps(tmp0, _b),
		tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
	vector3_t res;
	_mm_store_ps((float*)&res, _mm_fmsub_ps(tmp0, tmp1, tmp4));
	return res;
}

/* Adds a with b and stores result in a. */
extern inline void matrix_add(const matrix_t a, const matrix_t b, matrix_t res)
{
	__m256 _a0 = _mm256_load_ps(a), _a1 = _mm256_load_ps(a + 8);
	__m256 _b0 = _mm256_load_ps(b), _b1 = _mm256_load_ps(b + 8);
	_mm256_store_ps(res, _mm256_add_ps(_a0, _b0));
	_mm256_store_ps(res + 8, _mm256_add_ps(_a1, _b1));
}

/* Subtracts a with b and stores result in a. */
extern inline void matrix_sub(const matrix_t a, const matrix_t b, matrix_t res)
{
	__m256 _a0 = _mm256_load_ps(a), _a1 = _mm256_load_ps(a + 8);
	__m256 _b0 = _mm256_load_ps(b), _b1 = _mm256_load_ps(b + 8);
	_mm256_store_ps(res, _mm256_sub_ps(_a0, _b0));
	_mm256_store_ps(res + 8, _mm256_sub_ps(_a1, _b1));
}

/* Multiplies a with b and stores result in a. */
extern inline void matrix_multiply(const matrix_t a, const matrix_t b, matrix_t res)
{
	/* https://stackoverflow.com/a/18508113 */
	__m128 row0 = _mm_load_ps(&b[0]),
		row1 = _mm_load_ps(&b[4]),
		row2 = _mm_load_ps(&b[8]),
		row3 = _mm_load_ps(&b[12]);
	for (int i = 0; i < 4; i++) {
		__m128 col0 = _mm_set1_ps(INDEX_MATRIX(a, i, 0)),
			col1 = _mm_set1_ps(INDEX_MATRIX(a, i, 1)),
			col2 = _mm_set1_ps(INDEX_MATRIX(a, i, 2)),
			col3 = _mm_set1_ps(INDEX_MATRIX(a, i, 3));
		__m128 row = _mm_add_ps(
			_mm_add_ps(
				_mm_mul_ps(col0, row0),
				_mm_mul_ps(col1, row1)),
			_mm_add_ps(
				_mm_mul_ps(col2, row2),
				_mm_mul_ps(col3, row3)));
		_mm_store_ps(&res[i * 4], row);
	}
}

/* Creates translation matrix with vec and stores result in res */
extern inline void matrix_translation(vector3_t vec, matrix_t res)
{
	memset(res, 0, sizeof(matrix_t));

	INDEX_MATRIX(res, 0, 0) = 1.0F;
	INDEX_MATRIX(res, 1, 1) = 1.0F;
	INDEX_MATRIX(res, 2, 2) = 1.0F;
	INDEX_MATRIX(res, 3, 3) = 1.0F;

	INDEX_MATRIX(res, 3, 0) = vec.x;
	INDEX_MATRIX(res, 3, 1) = vec.y;
	INDEX_MATRIX(res, 3, 2) = vec.z;
}

typedef enum axis
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z
} axis_t;

typedef struct aabb
{
	vector3_t min, max;
} aabb_t;

extern inline bool aabb_collides_aabb(aabb_t a, aabb_t b)
{
	return a.min.x <= b.max.x && a.max.x >= b.min.x &&
		a.min.y <= b.max.y && a.max.y >= b.min.y &&
		a.min.z <= b.max.z && a.max.z >= b.min.z;
}

extern inline vector3_t aabb_get_center(aabb_t aabb)
{
	return vector3_mul_scalar(vector3_add(aabb.min, aabb.max), 0.5F);
}

extern inline aabb_t aabb_set_center(aabb_t aabb, vector3_t pos)
{
	vector3_t dim_2 = vector3_mul_scalar(vector3_sub(aabb.max, aabb.min), 0.5F);
	aabb.min = vector3_sub(pos, dim_2);
	aabb.max = vector3_add(pos, dim_2);
	return aabb;
}

extern inline aabb_t aabb_translate(aabb_t aabb, vector3_t pos)
{
	aabb.min = vector3_add(aabb.min, pos);
	aabb.max = vector3_add(aabb.max, pos);
	return aabb;
}

extern inline aabb_t aabb_translate_axis(aabb_t aabb, axis_t axis_i, float value)
{
	vector3_uarray_t axis = { 0 };
	axis.raw[axis_i] = value;
	return aabb_translate(aabb, axis.vec);
}

extern inline vector3_t aabb_dimensions(aabb_t aabb)
{
	return vector3_sub(aabb.max, aabb.min);
}

extern inline vector3_t aabb_collision_depth(aabb_t a, aabb_t b)
{
	vector3_uarray_t a_c = { aabb_get_center(a) },
		b_c = { aabb_get_center(b) };
	vector3_uarray_t res = { 0 };
	for (int i = 0; i < 3; i++)
	{
		if (a_c.raw[i] < b_c.raw[i])
		{
			res.raw[i] = (vector3_uarray_t){ a.max }.raw[i] - (vector3_uarray_t) { b.min }.raw[i];
		}
		else
		{
			res.raw[i] = (vector3_uarray_t){ b.max }.raw[i] - (vector3_uarray_t) { a.min }.raw[i];
		}
	}
	return res.vec;
}