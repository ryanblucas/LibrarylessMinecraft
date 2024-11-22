/*
	camera.c ~ RL
*/

#include "camera.h"
#include "graphics.h"
#include "window.h"
#include "world.h"

static matrix_t projection, view, view_projection;
static float yaw, pitch;
static vector3_t position;

static float near, far, fov, aspect;

void camera_set_projection_properties(float _near, float _far, float _fov, float _aspect)
{
	near = _near;
	far = _far;
	fov = _fov;
	aspect = _aspect;

	float tan_fov = tanf((float)M_PI_2 - fov / 2), /* 1 / tan(fov / 2) */
		inverse_z_range = 1.0F / (near - far);

	memset(projection, 0, sizeof(matrix_t));
	MAT4_AT(projection, 0, 0) = tan_fov / aspect;
	MAT4_AT(projection, 1, 1) = tan_fov;
	MAT4_AT(projection, 2, 2) = (near + far) * inverse_z_range;
	MAT4_AT(projection, 2, 3) = -1.0F;
	MAT4_AT(projection, 3, 2) = 2 * near * far * inverse_z_range;

	matrix_multiply(view, projection, view_projection);
}

void camera_set_view_properties(float _yaw, float _pitch, vector3_t pos)
{
	yaw = _yaw;
	pitch = _pitch;
	position = pos;

	vector3_t forward = vector3_mul_scalar(vector3_normalize((vector3_t) { cosf(yaw)* cosf(pitch), sinf(pitch), sinf(yaw)* cosf(pitch) }), -1.0F),
		right = vector3_normalize(vector3_cross((vector3_t) { 0, 1, 0 }, forward)),
		up = vector3_normalize(vector3_cross(forward, right));

	matrix_t res =
	{
		right.x, up.x, forward.x, 0.0F,
		right.y, up.y, forward.y, 0.0F,
		right.z, up.z, forward.z, 0.0F,
		-vector3_dot(position, right), -vector3_dot(position, up), -vector3_dot(position, forward), 1.0F
	};
	memcpy(view, res, sizeof res);
	matrix_multiply(view, projection, view_projection);
}

float camera_near(void)
{
	return near;
}

float camera_far(void)
{
	return far;
}

float camera_fov(void)
{
	return fov;
}

float camera_aspect(void)
{
	return aspect;
}

float camera_yaw(void)
{
	return yaw;
}

float camera_pitch(void)
{
	return pitch;
}

vector3_t camera_position(void)
{
	return position;
}

/* There should be caching here too. TO DO */

vector3_t camera_forward(void)
{
	return vector3_normalize((vector3_t) { cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch) });
}

vector3_t camera_right(void)
{
	return vector3_normalize(vector3_cross((vector3_t) { 0, 1, 0 }, camera_forward()));
}

vector3_t camera_up(void)
{
	return vector3_normalize(vector3_cross(camera_forward(), camera_right()));
}

void camera_projection(matrix_t out)
{
	memcpy(out, projection, sizeof projection);
}

void camera_view(matrix_t out)
{
	memcpy(out, view, sizeof view);
}

void camera_view_projection(matrix_t out)
{
	memcpy(out, view_projection, sizeof view_projection);
}

void camera_update(vector3_t pos)
{
	pointi_t dm = window_mouse_delta();
	float yaw = camera_yaw() + DEGREES_TO_RADIANS(dm.x),
		pitch = camera_pitch() - DEGREES_TO_RADIANS(dm.y);
	yaw = fmodf(2 * M_PI + yaw, 2 * M_PI);
	pitch = min(pitch, DEGREES_TO_RADIANS(89.9F));
	pitch = max(pitch, DEGREES_TO_RADIANS(-89.9F));
	camera_set_view_properties(yaw, pitch, pos);
}