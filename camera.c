/*
	camera.c ~ RL
*/

#include "camera.h"
#include "window.h"

/* The most common case is for only one camera, so having only one cache makes more sense. */
static struct camera_cache
{
	hash_t view_hash, proj_hash;
	matrix_t projection, view, view_projection;
	float aspect_ratio;
} current_cache;

camera_t current_camera = { .near = 0.1F, .far = 1000.0F, .fov = 90.0F };

static void camera_cache_projection(float aspect_ratio)
{
	float tan_fov = tanf((float)M_PI_2 - DEGREES_TO_RADIANS(current_camera.fov) / 2), /* 1 / tan(fov / 2) */
		inverse_z_range = 1.0F / (current_camera.near - current_camera.far);
	
	current_cache.aspect_ratio = aspect_ratio;
	memset(current_cache.projection, 0, sizeof(matrix_t));
	INDEX_MATRIX(current_cache.projection, 0, 0) = tan_fov / aspect_ratio;
	INDEX_MATRIX(current_cache.projection, 1, 1) = tan_fov;
	INDEX_MATRIX(current_cache.projection, 2, 2) = (current_camera.near + current_camera.far) * inverse_z_range;
	INDEX_MATRIX(current_cache.projection, 2, 3) = -1.0F;
	INDEX_MATRIX(current_cache.projection, 3, 2) = 2 * current_camera.near * current_camera.far * inverse_z_range;
}

bool camera_get_projection(matrix_t out)
{
	bool res = false;

	pointi_t dims = window_get_dimensions();
	float aspect_ratio = (float)dims.x / dims.y;
	hash_t proj_hash = mc_hash((uint8_t*)&current_camera + PROJECTION_MEMBERS_OFFSET, SIZEOF_PROJECTION_MEMBERS);
	if (current_cache.proj_hash != proj_hash || current_cache.aspect_ratio != aspect_ratio)
	{
		current_cache.proj_hash = proj_hash;
		camera_cache_projection(aspect_ratio);
		res = true;
	}
	memcpy(out, current_cache.projection, sizeof current_cache.projection);
	return res;
}

static void camera_cache_view(void)
{
	vector3_t forward = vector3_mul_scalar(vector3_normalize((vector3_t) { cosf(current_camera.yaw) * cosf(current_camera.pitch), sinf(current_camera.pitch), sinf(current_camera.yaw) * cosf(current_camera.pitch) }), -1.0F),
		right = vector3_normalize(vector3_cross((vector3_t) { 0, 1, 0 }, forward)),
		up = vector3_normalize(vector3_cross(forward, right));

	matrix_t res =
	{
		right.x, up.x, forward.x, 0.0F,
		right.y, up.y, forward.y, 0.0F,
		right.z, up.z, forward.z, 0.0F,
		-vector3_dot(current_camera.pos, right), -vector3_dot(current_camera.pos, up), -vector3_dot(current_camera.pos, forward), 1.0F
	};
	memcpy(current_cache.view, res, sizeof res);
}

bool camera_get_view(matrix_t out)
{
	bool res = false;

	hash_t view_hash = mc_hash((uint8_t*)&current_camera + VIEW_MEMBERS_OFFSET, SIZEOF_VIEW_MEMBERS);
	if (current_cache.view_hash != view_hash)
	{
		current_cache.view_hash = view_hash;
		camera_cache_view();
		res = true;
	}
	memcpy(out, current_cache.view, sizeof current_cache.view);
	return res;
}

bool camera_get_view_projection(matrix_t out)
{
	bool res = false;

	static matrix_t proj, view;
	if (camera_get_projection(proj) || camera_get_view(view))
	{
		matrix_multiply(view, proj, current_cache.view_projection);
		res = true;
	}
	memcpy(out, current_cache.view_projection, sizeof current_cache.view_projection);
	return res;
}

/* There should be caching here too. TO DO */

vector3_t camera_forward(void)
{
	return vector3_normalize((vector3_t) { cosf(current_camera.yaw) * cosf(current_camera.pitch), sinf(current_camera.pitch), sinf(current_camera.yaw) * cosf(current_camera.pitch) });
}

vector3_t camera_right(void)
{
	return vector3_normalize(vector3_cross((vector3_t) { 0, 1, 0 }, camera_forward()));
}

vector3_t camera_up(void)
{
	return vector3_normalize(vector3_cross(camera_forward(), camera_right()));
}