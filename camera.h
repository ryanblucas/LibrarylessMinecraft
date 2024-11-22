/*
	camera.h ~ RL
*/

#pragma once

#include <stdbool.h>
#include "util.h"

/* Sets camera's projection transform properties. FOV is in radians */
void camera_set_projection_properties(float near, float far, float fov, float aspect);
/* Sets the camera's view transform properties */
void camera_set_view_properties(float yaw, float pitch, vector3_t pos);

/* Gets camera's near Z */
float camera_near(void);
/* Gets camera's far Z */
float camera_far(void);
/* Gets camera's Y FOV in radians */
float camera_fov(void);
/* Returns camera's aspect ratio (width divided by height) */
float camera_aspect(void);

/* Gets camera's yaw rotation (X-axis) in radians */
float camera_yaw(void);
/* Gets camera's pitch rotation (Y-axis) in radians */
float camera_pitch(void);
/* Gets camera's position */
vector3_t camera_position(void);

/* Returns the forward vector */
vector3_t camera_forward(void);
/* Returns the right vector (which actually points to the left!!) */
vector3_t camera_right(void);
/* Returns the up vector */
vector3_t camera_up(void);

/* Sets out's contents to projection matrix */
void camera_projection(matrix_t out);
/* Sets out's contents to view matrix */
void camera_view(matrix_t out);
/* Sets out's contents to view * projection matrix */
void camera_view_projection(matrix_t out);

/* Updates camera */
void camera_update(vector3_t pos);