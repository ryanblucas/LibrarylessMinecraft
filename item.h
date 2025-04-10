/*
	item.h ~ RL
*/

#pragma once

#include <stdint.h>

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
	DEFINE_BLOCK(LOG) \
	DEFINE_BLOCK(LEAVES) \
	DEFINE_BLOCK(ORE_GOLD) \
	DEFINE_BLOCK(ORE_IRON) \
	DEFINE_BLOCK(ORE_COAL) \
	DEFINE_BLOCK(ORE_DIAMOND) \

	BLOCK_LIST
	BLOCK_COUNT
#undef DEFINE_BLOCK
};
typedef uint8_t block_type_t;

typedef struct inventory
{
	int active_slot;
	block_type_t items[45]; /* First 9 elements are hotbar, then the next 27 are from top to bottom the inventory. Next 4 are for armor, last 5 are for crafting grid */
} inventory_t;