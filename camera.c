/*
	camera.c ~ RL
*/

#include "camera.h"
#include "graphics.h"
#include "window.h"

static matrix_t projection, view, view_projection;
static float yaw, pitch;
static vector3_t position;

void camera_set_projection_properties(float near, float far, float fov)
{
	float tan_fov = tanf((float)M_PI_2 - DEGREES_TO_RADIANS(fov) / 2), /* 1 / tan(fov / 2) */
		inverse_z_range = 1.0F / (near - far);

	float aspect_ratio = (float)window_get_dimensions().x / window_get_dimensions().y;
	memset(projection, 0, sizeof(matrix_t));
	INDEX_MATRIX(projection, 0, 0) = tan_fov / aspect_ratio;
	INDEX_MATRIX(projection, 1, 1) = tan_fov;
	INDEX_MATRIX(projection, 2, 2) = (near + far) * inverse_z_range;
	INDEX_MATRIX(projection, 2, 3) = -1.0F;
	INDEX_MATRIX(projection, 3, 2) = 2 * near * far * inverse_z_range;

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

/* Uses camera in current graphics shader */
void camera_activate(void)
{
	graphics_shader_matrix("camera", view_projection);
}