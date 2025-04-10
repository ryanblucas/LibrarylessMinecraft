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
	if (*list)
	{
		free((*list)->array);
	}
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

#define MC_MAP_INDEX(map, i) (*(struct map_pair*)((char*)(map)->data + (i) * (sizeof(struct map_pair) + (map)->element_size)))

struct map
{
	size_t element_size;
	int reserved, count;
	struct map_pair
	{
		hash_t key;
		uint8_t value[];
	} *data;
};

void mc_map_iterate(const map_t map, map_iterate_func_t callback, void* user)
{
	for (int i = 0; i < map->reserved; i++)
	{
		if (MC_MAP_INDEX(map, i).key != 0)
		{
			if (!callback(map, MC_MAP_INDEX(map, i).key, MC_MAP_INDEX(map, i).value, user))
			{
				break;
			}
		}
	}
}

int mc_map_count(const map_t map)
{
	return map->count;
}

size_t mc_map_element_size(const map_t map)
{
	return map->element_size;
}

static inline struct map_pair* mc_map_pair(const map_t map, hash_t key)
{
	for (int i = key % map->reserved; i < map->reserved; i++)
	{
		if (MC_MAP_INDEX(map, i).key == key)
		{
			return &MC_MAP_INDEX(map, i);
		}
	}

	return NULL;
}

map_t mc_map_create(size_t element_size)
{
	map_t result = mc_malloc(sizeof * result);
	result->element_size = element_size;
	result->reserved = 96;
	result->count = 0;

	size_t sz_data = (sizeof(struct map_pair) + element_size) * result->reserved;
	result->data = mc_malloc(sz_data);
	memset(result->data, 0, sz_data);
	return result;
}

void mc_map_destroy(map_t* map)
{
	map_t _map = *map;
	if (*map)
	{
		free((*map)->data);
	}
	free(*map);
	*map = NULL;
}

static void mc_map_realloc(map_t map)
{
	struct map copy = *map;
	copy.count = 0;
	copy.reserved *= 2;

	size_t sz_data = (sizeof(struct map_pair) + copy.element_size) * copy.reserved;
	copy.data = mc_malloc(sz_data);
	memset(copy.data, 0, sz_data);

	for (int i = 0; i < map->reserved; i++)
	{
		if (MC_MAP_INDEX(map, i).key != 0)
		{
			mc_map_add(&copy, MC_MAP_INDEX(map, i).key, MC_MAP_INDEX(map, i).value, copy.element_size);
		}
	}

	free(map->data);
	*map = copy;
}

bool mc_map_add(map_t map, hash_t key, const void* pelement, size_t element_size)
{
	assert(element_size == map->element_size);
	struct map_pair* pair = mc_map_pair(map, key);
	bool res = !!pair;
	if (!pair)
	{
		int i;
		for (i = key % map->reserved; i < map->reserved && MC_MAP_INDEX(map, i).key != 0; i++);
		if (i >= map->reserved)
		{
			mc_map_realloc(map);
			mc_map_add(map, key, pelement, element_size);
			return false;
		}
		pair = &MC_MAP_INDEX(map, i);
	}

	pair->key = key;
	memcpy(pair->value, pelement, element_size);
	map->count++;
	return res;
}

void mc_map_remove(map_t map, hash_t key, void* out, size_t element_size)
{
	struct map_pair* pair = mc_map_pair(map, key);
	if (!pair)
	{
		return;
	}
	if (out)
	{
		assert(element_size == map->element_size);
		memcpy(out, pair->value, element_size);
	}
	pair->key = 0;
	map->count--;
}

void* mc_map_get(const map_t map, hash_t key, void* out, size_t element_size)
{
	struct map_pair* pair = mc_map_pair(map, key);
	if (!pair)
	{
		return NULL;
	}
	if (out)
	{
		assert(element_size == map->element_size);
		memcpy(out, pair->value, element_size);
	}
	return pair->value;
}

void mc_map_clear(map_t map)
{
	map->count = 0;
	memset(map->data, 0, map->element_size * map->reserved);
}

void mc_set_iterate(const hash_set_t set, set_iterate_func_t callback, void* user)
{
	for (int i = 0; i < set->reserved; i++)
	{
		if (MC_MAP_INDEX(set, i).key != 0)
		{
			if (!callback(set, MC_MAP_INDEX(set, i).value, user))
			{
				break;
			}
		}
	}
}

int mc_set_count(const hash_set_t list)
{
	return list->count;
}

size_t mc_set_element_size(const hash_set_t list)
{
	return list->element_size;
}

hash_set_t mc_set_create(size_t element_size)
{
	return mc_map_create(element_size);
}

void mc_set_destroy(hash_set_t* list)
{
	mc_map_destroy(list);
}

bool mc_set_add(hash_set_t list, const void* pelement, size_t element_size)
{
	assert(element_size == list->element_size);
	return mc_map_add(list, mc_hash(pelement, element_size), pelement, element_size);
}

void mc_set_remove(hash_set_t list, const void* pelement, size_t element_size)
{
	assert(element_size == list->element_size);
	mc_map_remove(list, mc_hash(pelement, element_size), NULL, element_size);
}

bool mc_set_has(hash_set_t list, const void* pelement, size_t element_size)
{
	assert(element_size == list->element_size);
	return !!mc_map_get(list, mc_hash(pelement, element_size), NULL, element_size);
}

void mc_set_clear(hash_set_t list)
{
	mc_map_clear(list);
}