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
/* Returns current tick's mouse wheel position (- for down, + for up) */
int window_mouse_wheel_position(void);
/* Returns the difference between last tick's wheel position and this current tick's wheel position */
int window_mouse_wheel_delta(void);

typedef enum input
{
	INPUT_FORWARD,
	INPUT_BACKWARD,
	INPUT_LEFT,
	INPUT_RIGHT,
	INPUT_SNEAK,
	INPUT_JUMP,

	INPUT_TELEPORT_TO_SPAWN,

	INPUT_BREAK_BLOCK,
	INPUT_PLACE_BLOCK,
	INPUT_CYCLE_BLOCK_FORWARD,
	INPUT_CYCLE_BLOCK_BACKWARD,
	INPUT_UPDATE_BLOCK,
	INPUT_QUEUE_BLOCK_INFO,
	INPUT_REGENERATE_WORLD,

	INPUT_TOGGLE_NOCLIP,
	INPUT_TOGGLE_MOUSE_FOCUS,
	INPUT_TOGGLE_WIREFRAME,
	INPUT_TOGGLE_CHUNK_BORDERS,
	INPUT_TOGGLE_VISUALIZE_AXIS,

	INPUT_COUNT
} input_t;

/* Updates input state */
void window_input_update(void);
/* Is the key currently pressed? */
bool window_input_down(input_t input);
/* Is the key clicked? Note: This is true when the tick before has this down and this tick has it up.
	So, if you call this in a render function, the results may be unpredictable. */
bool window_input_clicked(input_t input);