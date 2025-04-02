/*
	interface.h ~ RL
*/

#pragma once

void interface_init(void);
void interface_destroy(void);
void interface_draw(void);

int interface_get_max_hearts(void);
int interface_get_current_hearts(void);
void interface_set_max_hearts(int hearts);
void interface_set_current_hearts(int hearts);