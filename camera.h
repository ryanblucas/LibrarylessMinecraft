/*
	camera.h ~ RL
*/

#pragma once

#include <stdbool.h>
#include "util.h"

/* Sets camera's projection transform properties. Aspect ratio is inferred from window */
void camera_set_projection_properties(float near, float far, float fov);
/* Sets the camera's view transform properties */
void camera_set_view_properties(float yaw, float pitch, vector3_t pos);

/* Gets camera's yaw rotation (X-axis) */
float camera_yaw(void);
/* Gets camera's pitch rotation (Y-axis) */
float camera_pitch(void);
/* Gets camera's position */
vector3_t camera_position(void);

/* Returns the forward vector */
vector3_t camera_forward(void);
/* Returns the right vector (which actually points to the left!!) */
vector3_t camera_right(void);
/* Returns the up vector */
vector3_t camera_up(void);

/* Uses camera in current graphics shader */
void camera_activate(void);