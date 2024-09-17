/*
	window.h ~ RL
	Parses input and creates window
*/

#pragma once

#include <stdbool.h>

typedef struct pointi
{
	int x, y;
} pointi_t;

/* Retrieves a high-precision timestamp in seconds. */
double window_time(void);

/* Returns the dimensions of the window. */
pointi_t window_get_dimensions(void);

/* Returns the difference between last frame's mouse position and this frame's mouse position */
pointi_t window_mouse_delta(void);
/* Returns current mouse position */
pointi_t window_mouse_position(void);

typedef enum input
{
	/* Add any new inputs to window_input_update */

	INPUT_FORWARD,
	INPUT_BACKWARD,
	INPUT_LEFT,
	INPUT_RIGHT,
	INPUT_SPRINT,
	INPUT_JUMP,

	INPUT_TELEPORT_TO_SPAWN,

	INPUT_BREAK_BLOCK,
	INPUT_PLACE_BLOCK,

	INPUT_TOGGLE_MOUSE_FOCUS,
	INPUT_TOGGLE_WIREFRAME,

	INPUT_COUNT
} input_t;

/* Is the key currently pressed? */
bool window_input_down(input_t input);
/* Is the key clicked? Note: This is true when the frame before has this down and this frame has it up. */
bool window_input_clicked(input_t input);