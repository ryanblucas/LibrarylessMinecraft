/*
	camera.h ~ RL
*/

#pragma once

#include <stdbool.h>
#include "util.h"

/* TO DO: there a better way of doing this? You could store each type of members in a different 
	struct, but that adds overhead to modules that don't even care about the benefits it brings */

/* Size of projection members in camera struct in bytes */
#define SIZEOF_PROJECTION_MEMBERS	(sizeof(float) * 3)
/* Offset from initial struct to find projection members */
#define PROJECTION_MEMBERS_OFFSET	0
/* Size of view members in camera struct in bytes */
#define SIZEOF_VIEW_MEMBERS			(sizeof(float) * 2 + sizeof(vector3_t))
/* Offset from initial struct to find view members */
#define VIEW_MEMBERS_OFFSET			(sizeof(float) * 3)

/*	TO DO: is the struct necessary? You can have a bunch of functions to get and set 
	properties, but having the values immediately accessible is pretty nice. */

typedef struct camera
{
	float near, far, fov;	/* Projection settings. Aspect ratio can be calculated using window_get_dimensions() */
	float yaw, pitch;		/* View settings */
	vector3_t pos;			/* Position of camera */
} camera_t;

extern camera_t current_camera;

/* Gets projection matrix of current camera and stores result in float[16] array "out."
	Returns true if anything was recalculated, false if matrix was drawn from cache. */
bool camera_get_projection(matrix_t out);
/* Gets view matrix of current camera and stores result in float[16] array "out"
	Returns true if anything was recalculated, false if matrix was drawn from cache. */
bool camera_get_view(matrix_t out);
/* Gets view * projection matrix of current camera and stores result in float[16] array "out"
	Returns true if anything was recalculated, false if matrix was drawn from cache. */
bool camera_get_view_projection(matrix_t out);

/* Returns the forward vector */
vector3_t camera_forward(void);
/* Returns the right vector (which actually points to the left!!) */
vector3_t camera_right(void);
/* Returns the up vector */
vector3_t camera_up(void);