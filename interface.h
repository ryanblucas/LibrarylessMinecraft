/*
	interface.h ~ RL
*/

#pragma once

#include "graphics.h"
#include "item.h"
#include <stdbool.h>
#include "window.h"

typedef struct rectanglei
{
	int x, y, wx, wy;
} rectanglei_t;

extern inline bool rectangle_is_point_inside(rectanglei_t rect, pointi_t point)
{
	return point.x >= rect.x && point.y >= rect.y && point.x < rect.x + rect.wx && point.y < rect.y + rect.wy;
}

/* Initializes user interface */
void interface_init(sampler_t item_atlas);
/* Destroys objects used by user interface */
void interface_destroy(void);
/* Updates then draws user interface */
void interface_frame(void);

/* Gets max hearts displayed on user interface */
int interface_get_max_hearts(void);
/* Gets current hearts displayed on user interface */
int interface_get_current_hearts(void);
/* Sets amount of heart containers displayed on user interface */
void interface_set_max_hearts(int hearts);
/* Sets current hearts displayed on user interface */
void interface_set_current_hearts(int hearts);

/* Gets if inventory is currently open */
bool interface_is_inventory_open(void);
/* Sets inventory visibility state, true to open, false to close */
void interface_set_inventory_state(bool state);
/* Sets inventory for the interface to show on screen. The interface can change the contents of the items around. */
void interface_set_inventory(inventory_t* inventory);