/*
	game.h ~ RL
*/

#pragma once

#include "graphics.h"
#include <stdbool.h>
#include "world.h"

void game_init(void);
void game_destroy(void);
void game_frame(float delta);