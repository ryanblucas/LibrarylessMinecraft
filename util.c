/*
	util.c ~ RL
*/

#include "util.h"

struct array_list
{
	size_t element_size;
	int reserved, count;
	uint8_t* array;
};

int mc_list_count(const array_list_t block_vertex_list)
{
	return block_vertex_list->count;
}

size_t mc_list_element_size(const array_list_t block_vertex_list)
{
	return block_vertex_list->element_size;
}

void* mc_list_array(array_list_t block_vertex_list)
{
	return block_vertex_list->array;
}

array_list_t mc_list_create(size_t element_size)
{
	struct array_list* res = mc_malloc(sizeof * res);
	res->reserved = 64;
	res->element_size = element_size;
	res->count = 0;
	res->array = mc_malloc(element_size * res->reserved);
	return res;
}

void mc_list_destroy(array_list_t* block_vertex_list)
{
	free((*block_vertex_list)->array);
	free(*block_vertex_list);
	*block_vertex_list = NULL;
}

int mc_list_add(array_list_t block_vertex_list, int index, const void* pelement, size_t element_size)
{
	assert(element_size == block_vertex_list->element_size);
	index = max(min(block_vertex_list->count, index), 0);

	if (++block_vertex_list->count >= block_vertex_list->reserved)
	{
		int new_reserved = block_vertex_list->reserved * 2;
		uint8_t* new_arr = mc_malloc(element_size * new_reserved);
		memcpy(new_arr, block_vertex_list->array, element_size * block_vertex_list->reserved);
		free(block_vertex_list->array);
		block_vertex_list->array = new_arr;
		block_vertex_list->reserved = new_reserved;
	}

	memmove(block_vertex_list->array + (index + 1) * element_size, block_vertex_list->array + index * element_size, (block_vertex_list->count - index - 1) * element_size);
	if (pelement)
	{
		memcpy(block_vertex_list->array + index * element_size, pelement, element_size);
	}
	else
	{
		memset(block_vertex_list->array + index * element_size, 0, element_size);
	}
	return index;
}

int mc_list_remove(array_list_t block_vertex_list, int index, void* out, size_t element_size)
{
	if (block_vertex_list->count == 0)
	{
		return -1;
	}

	index = max(min(block_vertex_list->count, index), 0);
	if (out)
	{
		assert(element_size == block_vertex_list->element_size);
		memcpy(out, block_vertex_list->array + element_size * index, element_size);
	}
	element_size = block_vertex_list->element_size;
	memmove(block_vertex_list->array + element_size * index, block_vertex_list->array + element_size * (index + 1), element_size);
	block_vertex_list->count--;
	return index;
}

void mc_list_splice(array_list_t block_vertex_list, int start, int count)
{
	if (count < 0)
	{
		start += count;
		count = -count;
	}
	if (count == 0 || start < 0 || start >= block_vertex_list->count)
	{
		return;
	}

	int end = min(start + count, block_vertex_list->count);
	memmove(block_vertex_list->array + start * block_vertex_list->element_size, block_vertex_list->array + end * block_vertex_list->element_size, (block_vertex_list->count - end) * block_vertex_list->element_size);
	block_vertex_list->count -= count;
}

void mc_list_array_add(array_list_t block_vertex_list, int index, void* arr, size_t element_size, int arr_size)
{
	assert(element_size == block_vertex_list->element_size && arr_size >= 0);
	index = max(min(block_vertex_list->count, index), 0);

	if (block_vertex_list->count + arr_size >= block_vertex_list->reserved)
	{
		int new_reserved = (1 << (int)(log2(block_vertex_list->count + arr_size) + 1.0));
		uint8_t* new_arr = mc_malloc(element_size * new_reserved);
		memcpy(new_arr, block_vertex_list->array, element_size * block_vertex_list->count);
		free(block_vertex_list->array);
		block_vertex_list->array = new_arr;
		block_vertex_list->reserved = new_reserved;
	}

	memmove(block_vertex_list->array + (index + arr_size) * element_size, block_vertex_list->array + index * element_size, (block_vertex_list->count - index) * element_size);
	memcpy(block_vertex_list->array + index * element_size, arr, element_size * arr_size);
	block_vertex_list->count += arr_size;
}