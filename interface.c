/*
	interface.c ~ RL
*/

#include "interface.h"
#include "entity.h"
#include "graphics.h"
#include "window.h"

static shader_t shader;
static sampler_t atlas;
static int atlas_width, atlas_height;

static vertex_buffer_t buffer;
static bool invalidate;

static int width, height;

static int max_hearts;
static int hearts;

void interface_init(void)
{
	shader = graphics_shader_load("assets/shaders/interface_vertex.glsl", "assets/shaders/interface_fragment.glsl");
	atlas = graphics_sampler_load("assets/interface.bmp");
	buffer = graphics_buffer_create(0, 0, VERTEX_STANDARD);

	atlas_width = graphics_sampler_width(atlas);
	atlas_height = graphics_sampler_height(atlas);

	max_hearts = PLAYER_HEART_COUNT;
	invalidate = true;
}

void interface_destroy(void)
{
	graphics_shader_delete(&shader);
	graphics_sampler_delete(&atlas);
	graphics_buffer_delete(&buffer);
}

static void interface_push_square(array_list_t vertices, vector3_t pos, float wx, float wy, float up, float vp, float ud, float vd)
{
	vertex_t a, b, c, d;

	a.x = pos.x;
	a.y = pos.y;
	a.z = pos.z;
	a.tx = up / atlas_width;
	a.ty = vp / atlas_height;

	b.x = pos.x;
	b.y = pos.y + wy;
	b.z = pos.z;
	b.tx = up / atlas_width;
	b.ty = (vp + vd) / atlas_height;

	c.x = pos.x + wx;
	c.y = pos.y + wy;
	c.z = pos.z;
	c.tx = (up + ud) / atlas_width;
	c.ty = (vp + vd) / atlas_height;

	d.x = pos.x + wx;
	d.y = pos.y;
	d.z = pos.z;
	d.tx = (up + ud) / atlas_width;
	d.ty = vp / atlas_height;

	mc_list_add(vertices, mc_list_count(vertices), &a, sizeof a);
	mc_list_add(vertices, mc_list_count(vertices), &b, sizeof b);
	mc_list_add(vertices, mc_list_count(vertices), &c, sizeof c);
	mc_list_add(vertices, mc_list_count(vertices), &c, sizeof c);
	mc_list_add(vertices, mc_list_count(vertices), &d, sizeof d);
	mc_list_add(vertices, mc_list_count(vertices), &a, sizeof a);
}

#define HEART_SIZE		36
#define HEART_SPACE_X	4

static void interface_invalidate_hearts(array_list_t vertices)
{
	int max_hearts_2 = (max_hearts + 1) / 2,
		hearts_2 = hearts / 2;
	int bar_width = HEART_SIZE * max_hearts_2 + HEART_SPACE_X * (max_hearts_2 - 1);
	for (int i = 0; i < max_hearts_2; i++)
	{
		interface_push_square(vertices, (vector3_t) { (width / 2 - bar_width / 2) + HEART_SIZE * i + HEART_SPACE_X * (i - 1), 0, 0.9 }, HEART_SIZE, HEART_SIZE, 18, 0, 9, 9);
	}
	int x = 0;
	for (int i = 0; i < hearts_2; i++)
	{
		x = (width / 2 - bar_width / 2) + HEART_SIZE * i + HEART_SPACE_X * (i - 1);
		interface_push_square(vertices, (vector3_t) { x, 0, 0 }, HEART_SIZE, HEART_SIZE, 0, 0, 9, 9);
	}

	if (hearts % 2 != 0)
	{
		x += HEART_SIZE + HEART_SPACE_X;
		interface_push_square(vertices, (vector3_t) { x, 0, 0 }, HEART_SIZE, HEART_SIZE, 9, 0, 9, 9);
	}
}

void interface_draw(void)
{
	graphics_clear_depth();

	graphics_sampler_use(atlas);
	graphics_shader_use(shader);

	pointi_t dimensions = window_get_dimensions();
	if (width != dimensions.x)
	{
		width = dimensions.x;
		graphics_shader_int("width", width);
		invalidate = true;
	}
	if (height != dimensions.y)
	{
		height = dimensions.y;
		graphics_shader_int("height", height);
		invalidate = true;
	}

	if (invalidate)
	{
		array_list_t vertices = mc_list_create(sizeof(vertex_t));

		interface_invalidate_hearts(vertices);

		graphics_buffer_modify(buffer, mc_list_array(vertices), mc_list_count(vertices));
		mc_list_destroy(&vertices);
		invalidate = false;
	}

	graphics_buffer_draw(buffer);
}

int interface_get_max_hearts(void)
{
	return max_hearts;
}

int interface_get_current_hearts(void)
{
	return hearts;
}

void interface_set_max_hearts(int hearts)
{
	max_hearts = hearts;
	invalidate = true;
}

void interface_set_current_hearts(int _hearts)
{
	hearts = _hearts;
	invalidate = true;
}