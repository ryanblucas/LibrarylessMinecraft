/*
	interface.c ~ RL
*/

#include "interface.h"
#include "entity.h"
#include "graphics.h"
#include "window.h"

#include "opengl.h"

static shader_t shader;
static sampler_t atlas;
static int atlas_width, atlas_height;

static vertex_buffer_t buffer;
static bool invalidate;

static int width, height;

static int max_hearts;
static int hearts;

static bool show_inventory;
static inventory_t* inventory;
static hash_t inventory_previous;

void interface_init(void)
{
	shader = graphics_shader_load("assets/shaders/interface_vertex.glsl", "assets/shaders/interface_fragment.glsl");
	atlas = graphics_sampler_load("assets/interface.bmp");
	buffer = graphics_buffer_create(0, 0, VERTEX_INTERFACE);

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
	interface_vertex_t a, b, c, d;

	a.color = b.color = c.color = d.color = COLOR_CREATE(255, 255, 255);

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

	mc_list_add(vertices, mc_list_count(vertices), &b, sizeof b);
	mc_list_add(vertices, mc_list_count(vertices), &a, sizeof a);
	mc_list_add(vertices, mc_list_count(vertices), &d, sizeof d);
	mc_list_add(vertices, mc_list_count(vertices), &d, sizeof d);
	mc_list_add(vertices, mc_list_count(vertices), &c, sizeof c);
	mc_list_add(vertices, mc_list_count(vertices), &b, sizeof b);
}

#define UI_SCALE		3

#define HEART_SIZE		(9 * UI_SCALE)
#define HEART_SPACE_X	0

#define REAL_BAR_WIDTH		182
#define REAL_BAR_HEIGHT		22
#define BAR_WIDTH			(REAL_BAR_WIDTH * UI_SCALE)
#define BAR_HEIGHT			(REAL_BAR_HEIGHT * UI_SCALE)
#define REAL_CURRENT_WIDTH	24
#define REAL_CURRENT_HEIGHT	23
#define CURRENT_WIDTH		(REAL_CURRENT_WIDTH * UI_SCALE)
#define CURRENT_HEIGHT		(REAL_CURRENT_HEIGHT * UI_SCALE)

static void interface_invalidate_hearts(array_list_t vertices)
{
	int y = height - BAR_HEIGHT - HEART_SIZE - UI_SCALE;
	int max_hearts_2 = (max_hearts + 1) / 2,
		hearts_2 = hearts / 2;
	for (int i = 0; i < max_hearts_2; i++)
	{
		interface_push_square(vertices, (vector3_t) { (width / 2 - BAR_WIDTH / 2) + HEART_SIZE * i + HEART_SPACE_X * (i - 1), y, -0.8F }, HEART_SIZE, HEART_SIZE, 18, 0, 9, 9);
	}
	int x = 0;
	for (int i = 0; i < hearts_2; i++)
	{
		x = (width / 2 - BAR_WIDTH / 2) + HEART_SIZE * i + HEART_SPACE_X * (i - 1);
		interface_push_square(vertices, (vector3_t) { x, y, -0.9F }, HEART_SIZE, HEART_SIZE, 0, 0, 9, 9);
	}

	if (hearts % 2 != 0 && hearts > 0)
	{
		x += HEART_SIZE + HEART_SPACE_X;
		interface_push_square(vertices, (vector3_t) { x, y, -0.9F }, HEART_SIZE, HEART_SIZE, 9, 0, 9, 9);
	}
}

static void interface_invalidate_hotbar(array_list_t vertices)
{
	interface_push_square(vertices, (vector3_t) { width / 2 - BAR_WIDTH / 2, height - BAR_HEIGHT, -0.8F }, BAR_WIDTH, BAR_HEIGHT, 0, 9, REAL_BAR_WIDTH, REAL_BAR_HEIGHT);

	if (!inventory)
	{
		return;
	}

	interface_push_square(vertices, (vector3_t) { width / 2 - BAR_WIDTH / 2 + inventory->active_slot * (BAR_WIDTH / 9) - UI_SCALE, height - BAR_HEIGHT - UI_SCALE, -0.9F },
		CURRENT_WIDTH, CURRENT_HEIGHT, 182, 0, REAL_CURRENT_WIDTH, REAL_CURRENT_HEIGHT);
}

void interface_frame(void)
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

	hash_t inventory_hash = mc_hash(inventory, sizeof * inventory);
	invalidate |= inventory_hash != inventory_previous;
	inventory_hash = inventory_previous;

	if (invalidate)
	{
		array_list_t vertices = mc_list_create(sizeof(interface_vertex_t));

		interface_invalidate_hearts(vertices);
		interface_invalidate_hotbar(vertices);

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

bool interface_is_inventory_open(void)
{
	return show_inventory;
}

void interface_set_inventory_state(bool state)
{
	show_inventory = state;
	invalidate = true;
}

void interface_set_inventory(inventory_t* _inventory)
{
	inventory = _inventory;
	invalidate = true;
}