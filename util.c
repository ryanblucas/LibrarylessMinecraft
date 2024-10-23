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

int mc_list_count(const array_list_t list)
{
	return list->count;
}

size_t mc_list_element_size(const array_list_t list)
{
	return list->element_size;
}

void* mc_list_array(array_list_t list)
{
	return list->array;
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

void mc_list_destroy(array_list_t* list)
{
	free((*list)->array);
	free(*list);
	*list = NULL;
}

int mc_list_add(array_list_t list, int index, const void* pelement, size_t element_size)
{
	assert(element_size == list->element_size);
	index = max(min(list->count, index), 0);

	if (++list->count >= list->reserved)
	{
		int new_reserved = list->reserved * 2;
		uint8_t* new_arr = mc_malloc(element_size * new_reserved);
		memcpy(new_arr, list->array, element_size * list->reserved);
		free(list->array);
		list->array = new_arr;
		list->reserved = new_reserved;
	}

	memmove(list->array + (index + 1) * element_size, list->array + index * element_size, (list->count - index - 1) * element_size);
	if (pelement)
	{
		memcpy(list->array + index * element_size, pelement, element_size);
	}
	else
	{
		memset(list->array + index * element_size, 0, element_size);
	}
	return index;
}

int mc_list_remove(array_list_t list, int index, void* out, size_t element_size)
{
	if (list->count == 0)
	{
		return -1;
	}

	index = max(min(list->count, index), 0);
	if (out)
	{
		assert(element_size == list->element_size);
		memcpy(out, list->array + element_size * index, element_size);
	}
	element_size = list->element_size;
	memmove(list->array + element_size * index, list->array + element_size * (index + 1), element_size);
	list->count--;
	return index;
}

void mc_list_splice(array_list_t list, int start, int count)
{
	if (count < 0)
	{
		start += count;
		count = -count;
	}
	if (count == 0 || start < 0 || start >= list->count)
	{
		return;
	}

	int end = min(start + count, list->count);
	memmove(list->array + start * list->element_size, list->array + end * list->element_size, (list->count - end) * list->element_size);
	list->count -= count;
}

void mc_list_array_add(array_list_t list, int index, void* arr, size_t element_size, int arr_size)
{
	assert(element_size == list->element_size && arr_size >= 0);
	index = max(min(list->count, index), 0);

	if (list->count + arr_size >= list->reserved)
	{
		int new_reserved = (1 << (int)(log2(list->count + arr_size) + 1.0));
		uint8_t* new_arr = mc_malloc(element_size * new_reserved);
		memcpy(new_arr, list->array, element_size * list->count);
		free(list->array);
		list->array = new_arr;
		list->reserved = new_reserved;
	}

	memmove(list->array + (index + arr_size) * element_size, list->array + index * element_size, (list->count - index) * element_size);
	memcpy(list->array + index * element_size, arr, element_size * arr_size);
	list->count += arr_size;
}