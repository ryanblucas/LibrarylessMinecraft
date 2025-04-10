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

static vertex_buffer_t hotbar_items;
static bool show_inventory;
static inventory_t* inventory;
static hash_t inventory_previous;

static rectanglei_t armor_inventory, main_inventory, hotbar_inventory;

static sampler_t items;
static int items_width, items_height;

static vertex_buffer_t dynamic, dynamic_item;
static int grabbed_item_index = -1;
static vector3_t grabbed_item_offset;

void interface_init(sampler_t item_atlas)
{
	shader = graphics_shader_load("assets/shaders/interface_vertex.glsl", "assets/shaders/interface_fragment.glsl");
	atlas = graphics_sampler_load("assets/interface.bmp");

	buffer = graphics_buffer_create(0, 0, VERTEX_INTERFACE);
	hotbar_items = graphics_buffer_create(0, 0, VERTEX_INTERFACE);
	dynamic = graphics_buffer_create(0, 0, VERTEX_INTERFACE);
	dynamic_item = graphics_buffer_create(0, 0, VERTEX_INTERFACE);

	atlas_width = graphics_sampler_width(atlas);
	atlas_height = graphics_sampler_height(atlas);

	max_hearts = PLAYER_HEART_COUNT;
	invalidate = true;

	items = item_atlas;
	items_width = graphics_sampler_width(items);
	items_height = graphics_sampler_height(items);
}

void interface_destroy(void)
{
	graphics_shader_delete(&shader);
	graphics_sampler_delete(&atlas);

	graphics_buffer_delete(&dynamic_item);
	graphics_buffer_delete(&dynamic);
	graphics_buffer_delete(&hotbar_items);
	graphics_buffer_delete(&buffer);
}

static void interface_push_square(array_list_t vertices, int atlas_wx, int atlas_wy, vector3_t pos, float wx, float wy, float up, float vp, float ud, float vd)
{
	interface_vertex_t a, b, c, d;

	a.color = b.color = c.color = d.color = COLOR_CREATE(255, 255, 255);

	a.x = pos.x;
	a.y = pos.y;
	a.z = pos.z;
	a.tx = up / atlas_wx;
	a.ty = vp / atlas_wy;

	b.x = pos.x;
	b.y = pos.y + wy;
	b.z = pos.z;
	b.tx = up / atlas_wx;
	b.ty = (vp + vd) / atlas_wy;

	c.x = pos.x + wx;
	c.y = pos.y + wy;
	c.z = pos.z;
	c.tx = (up + ud) / atlas_wx;
	c.ty = (vp + vd) / atlas_wy;

	d.x = pos.x + wx;
	d.y = pos.y;
	d.z = pos.z;
	d.tx = (up + ud) / atlas_wx;
	d.ty = vp / atlas_wy;

	mc_list_add(vertices, mc_list_count(vertices), &b, sizeof b);
	mc_list_add(vertices, mc_list_count(vertices), &a, sizeof a);
	mc_list_add(vertices, mc_list_count(vertices), &d, sizeof d);
	mc_list_add(vertices, mc_list_count(vertices), &d, sizeof d);
	mc_list_add(vertices, mc_list_count(vertices), &c, sizeof c);
	mc_list_add(vertices, mc_list_count(vertices), &b, sizeof b);
}

static inline void interface_push_ui_square(array_list_t vertices, vector3_t pos, float wx, float wy, float up, float vp, float ud, float vd)
{
	interface_push_square(vertices, atlas_width, atlas_height, pos, wx, wy, up, vp, ud, vd);
}

static inline void interface_push_item_square(array_list_t vertices, vector3_t pos, float wx, float wy, float up, float vp, float ud, float vd)
{
	interface_push_square(vertices, items_width, items_height, pos, wx, wy, up, vp, ud, vd);
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

#define REAL_INVENTORY_WIDTH	168
#define REAL_INVENTORY_HEIGHT	158
#define INVENTORY_WIDTH			(REAL_INVENTORY_WIDTH * UI_SCALE)
#define INVENTORY_HEIGHT		(REAL_INVENTORY_HEIGHT * UI_SCALE)
#define REAL_SLOT_WIDTH			18
#define REAL_SLOT_HEIGHT		18
#define SLOT_WIDTH				(REAL_SLOT_WIDTH * UI_SCALE)
#define SLOT_HEIGHT				(REAL_SLOT_HEIGHT * UI_SCALE)

/* Creates panel, as seen in the inventory UI. The wx and wy arguments
	are for the size of the inner panel, not including the corners.
	Returns the top-left corner of the panel. */
static vector3_t interface_create_panel(array_list_t vertices, vector3_t pos, float wx, float wy)
{
	/* Corners */
	interface_push_ui_square(vertices, pos, UI_SCALE * 4, UI_SCALE * 4, 80, 31, 4, 4);
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { wx + UI_SCALE * 4 }), UI_SCALE * 4, UI_SCALE * 4, 108, 31, 4, 4);
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { 0, wy + UI_SCALE * 4 }), UI_SCALE * 4, UI_SCALE * 4, 80, 59, 4, 4);
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { wx + UI_SCALE * 4, wy + UI_SCALE * 4 }), UI_SCALE * 4, UI_SCALE * 4, 108, 59, 4, 4);

	/* Sides */
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { UI_SCALE * 4 }), wx, UI_SCALE * 4, 84, 31, 24, 4);
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { 0, UI_SCALE * 4 }), UI_SCALE * 4, wy, 80, 35, 4, 24);
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { UI_SCALE * 4, UI_SCALE * 4 + wy }), wx, UI_SCALE * 4, 84, 59, 24, 4);
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { UI_SCALE * 4 + wx, UI_SCALE * 4 }), UI_SCALE * 4, wy, 108, 35, 4, 24);

	/* Filling it in */
	interface_push_ui_square(vertices, vector3_add(pos, (vector3_t) { UI_SCALE * 4, UI_SCALE * 4 }), wx, wy, 84, 35, 24, 24);

	return vector3_add(pos, (vector3_t) { UI_SCALE * 4, UI_SCALE * 4 });
}

static void interface_invalidate_hearts(array_list_t vertices)
{
	int y = height - BAR_HEIGHT - HEART_SIZE - UI_SCALE;
	int max_hearts_2 = (max_hearts + 1) / 2,
		hearts_2 = hearts / 2;
	for (int i = 0; i < max_hearts_2; i++)
	{
		interface_push_ui_square(vertices, (vector3_t) { (width / 2 - BAR_WIDTH / 2) + HEART_SIZE * i + HEART_SPACE_X * (i - 1), y, 0.9F }, HEART_SIZE, HEART_SIZE, 18, 0, 9, 9);
	}
	int x = 0;
	for (int i = 0; i < hearts_2; i++)
	{
		x = (width / 2 - BAR_WIDTH / 2) + HEART_SIZE * i + HEART_SPACE_X * (i - 1);
		interface_push_ui_square(vertices, (vector3_t) { x, y, 0.8F }, HEART_SIZE, HEART_SIZE, 0, 0, 9, 9);
	}

	if (hearts % 2 != 0 && hearts > 0)
	{
		x += HEART_SIZE + HEART_SPACE_X;
		interface_push_ui_square(vertices, (vector3_t) { x, y, 0.8F }, HEART_SIZE, HEART_SIZE, 9, 0, 9, 9);
	}
}

static void interface_invalidate_hotbar(array_list_t vertices, array_list_t hotbar_items)
{
	if (!inventory)
	{
		return;
	}

	interface_push_ui_square(vertices, (vector3_t) { width / 2 - BAR_WIDTH / 2, height - BAR_HEIGHT, 0.9F }, BAR_WIDTH, BAR_HEIGHT, 0, 9, REAL_BAR_WIDTH, REAL_BAR_HEIGHT);
	interface_push_ui_square(vertices, (vector3_t) { width / 2 - BAR_WIDTH / 2 + inventory->active_slot * (BAR_WIDTH / 9) - UI_SCALE, height - BAR_HEIGHT - UI_SCALE, 0.8F },
		CURRENT_WIDTH, CURRENT_HEIGHT, 182, 0, REAL_CURRENT_WIDTH, REAL_CURRENT_HEIGHT);
	
	for (int i = 0; i < 9; i++)
	{
		block_type_t type = inventory->items[i];
		if (type != BLOCK_AIR)
		{
			type--; /* because of BLOCK_AIR not being in atlas */
			interface_push_item_square(hotbar_items, (vector3_t) { width / 2 - BAR_WIDTH / 2 + 3 * UI_SCALE + i * (20 * UI_SCALE), height - BAR_HEIGHT + 3 * UI_SCALE, 0.7F },
				16 * UI_SCALE, 16 * UI_SCALE, type * 16, 0, 16, 16);
		}
	}

	if (show_inventory)
	{
		/* background color */
		interface_push_ui_square(vertices, (vector3_t) { 0, 0, 0.0F }, width, height, 0, 44, 1, 1);
		
		vector3_t origin = interface_create_panel(vertices, (vector3_t) { width / 2 - INVENTORY_WIDTH / 2, height / 2 - INVENTORY_HEIGHT / 2, -0.1F }, INVENTORY_WIDTH, INVENTORY_HEIGHT);
		vector3_t armor = (vector3_t){ origin.x + UI_SCALE * 3, origin.y + UI_SCALE * 3, -0.2F };

		armor_inventory.x = armor.x;
		armor_inventory.y = armor.y;
		armor_inventory.wx = SLOT_WIDTH;
		armor_inventory.wy = SLOT_HEIGHT * 4;

		for (int i = 0; i < 4; i++)
		{
			interface_push_ui_square(vertices, armor, SLOT_WIDTH, SLOT_HEIGHT, 188, 23, REAL_SLOT_WIDTH, REAL_SLOT_HEIGHT);
			interface_push_ui_square(vertices, vector3_add(armor, (vector3_t) { UI_SCALE, UI_SCALE, -0.1F }), 16 * UI_SCALE, 16 * UI_SCALE, 16 + 16 * i, 31, 16, 16);
			armor.y += SLOT_HEIGHT;
		}

		vector3_t inv = (vector3_t){ origin.x + UI_SCALE * 3, origin.y + UI_SCALE * 78, -0.2F };

		main_inventory.x = inv.x;
		main_inventory.y = inv.y;
		main_inventory.wx = SLOT_WIDTH * 9;
		main_inventory.wy = SLOT_HEIGHT * 3;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 9; j++)
			{
				block_type_t item = inventory->items[(i + 1) * 9 + j];
				if (item != BLOCK_AIR && 9 + i * 9 + j != grabbed_item_index)
				{
					item--;
					interface_push_item_square(hotbar_items, vector3_add(inv, (vector3_t) { UI_SCALE, UI_SCALE, -0.2F }), 16 * UI_SCALE, 16 * UI_SCALE, item * 16, 0, 16, 16);
				}
				interface_push_ui_square(vertices, inv, SLOT_WIDTH, SLOT_HEIGHT, 188, 23, REAL_SLOT_WIDTH, REAL_SLOT_HEIGHT);
				inv.x += SLOT_WIDTH;
			}
			inv.x = (float)main_inventory.x;
			inv.y += SLOT_HEIGHT;
		}

		inv.x = origin.x + UI_SCALE * 3;
		inv.y += UI_SCALE * 3;

		hotbar_inventory.x = inv.x;
		hotbar_inventory.y = inv.y;
		hotbar_inventory.wx = SLOT_WIDTH * 9;
		hotbar_inventory.wy = SLOT_HEIGHT;

		for (int i = 0; i < 9; i++)
		{
			block_type_t item = inventory->items[i];
			if (item != BLOCK_AIR && i != grabbed_item_index)
			{
				item--;
				interface_push_item_square(hotbar_items, vector3_add(inv, (vector3_t) { UI_SCALE, UI_SCALE, -0.2F }), 16 * UI_SCALE, 16 * UI_SCALE, item * 16, 0, 16, 16);
			}
			interface_push_ui_square(vertices, inv, SLOT_WIDTH, SLOT_HEIGHT, 188, 23, REAL_SLOT_WIDTH, REAL_SLOT_HEIGHT);
			inv.x += SLOT_WIDTH;
		}
	}
}

static inline vector3_t interface_inventory_check_region(rectanglei_t region, pointi_t mouse_pos)
{
	vector3_t pos = { 0 };
	if (rectangle_is_point_inside(region, mouse_pos))
	{
		pos.z = -0.9F;
		mouse_pos.x -= region.x;
		mouse_pos.y -= region.y;
		mouse_pos.x /= SLOT_WIDTH;
		mouse_pos.y /= SLOT_HEIGHT;
		pos.x = region.x + mouse_pos.x * SLOT_WIDTH;
		pos.y = region.y + mouse_pos.y * SLOT_WIDTH;
	}
	return pos;
}

static void interface_inventory_do_dynamic(array_list_t vertices, array_list_t items_vertices)
{
	pointi_t mouse_pos = window_mouse_position();
	vector3_t pos = { 0 };

	pos = vector3_add(pos, interface_inventory_check_region(main_inventory, mouse_pos));
	pos = vector3_add(pos, interface_inventory_check_region(hotbar_inventory, mouse_pos));
	pos = vector3_add(pos, interface_inventory_check_region(armor_inventory, mouse_pos));

	if (pos.z != 0.0F)
	{
		interface_push_ui_square(vertices, pos, SLOT_WIDTH, SLOT_HEIGHT, 1, 44, 1, 1);
	}

	if (grabbed_item_index != -1)
	{
		block_type_t item = inventory->items[grabbed_item_index] - 1;
		vector3_t mouse_pos_v3 = { mouse_pos.x, mouse_pos.y, -0.95F };
		interface_push_item_square(items_vertices, vector3_sub(mouse_pos_v3, grabbed_item_offset), 16 * UI_SCALE, 16 * UI_SCALE, item * 16, 0, 16, 16);
	}
}

void interface_render(void)
{
	graphics_clear_depth();

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
		array_list_t hotbar_items_vertices = mc_list_create(sizeof(interface_vertex_t));

		interface_invalidate_hearts(vertices);
		interface_invalidate_hotbar(vertices, hotbar_items_vertices);

		graphics_buffer_modify(buffer, mc_list_array(vertices), mc_list_count(vertices));
		graphics_buffer_modify(hotbar_items, mc_list_array(hotbar_items_vertices), mc_list_count(hotbar_items_vertices));

		mc_list_destroy(&vertices);
		mc_list_destroy(&hotbar_items_vertices);
		invalidate = false;
	}

	graphics_sampler_use(items);
	graphics_buffer_draw(hotbar_items);

	graphics_sampler_use(atlas);
	graphics_buffer_draw(buffer);

	if (show_inventory)
	{
		array_list_t vertices = mc_list_create(sizeof(interface_vertex_t));
		array_list_t items_vertices = mc_list_create(sizeof(interface_vertex_t));

		interface_inventory_do_dynamic(vertices, items_vertices);

		graphics_buffer_modify(dynamic, mc_list_array(vertices), mc_list_count(vertices));
		graphics_buffer_modify(dynamic_item, mc_list_array(items_vertices), mc_list_count(items_vertices));

		mc_list_destroy(&vertices);
		mc_list_destroy(&items_vertices);

		graphics_buffer_draw(dynamic);
		graphics_sampler_use(items);
		graphics_buffer_draw(dynamic_item);
	}
}

void interface_update(void)
{
	if (show_inventory)
	{
		if (window_input_clicked(INPUT_INTERFACE_INTERACT))
		{
			pointi_t mouse_pos = window_mouse_position();
			int slot = -1;
			if (rectangle_is_point_inside(main_inventory, mouse_pos))
			{
				mouse_pos.x -= main_inventory.x;
				mouse_pos.y -= main_inventory.y;
				pointi_t temp = mouse_pos;
				mouse_pos.x /= SLOT_WIDTH;
				mouse_pos.y /= SLOT_HEIGHT;
				grabbed_item_offset = (vector3_t){ temp.x - mouse_pos.x * SLOT_WIDTH, temp.y - mouse_pos.y * SLOT_HEIGHT };
				slot = 9 + mouse_pos.y * 9 + mouse_pos.x;
			}
			else if (rectangle_is_point_inside(hotbar_inventory, mouse_pos))
			{
				mouse_pos.x -= hotbar_inventory.x;
				mouse_pos.y -= hotbar_inventory.y;
				pointi_t temp = mouse_pos;
				mouse_pos.x /= SLOT_WIDTH;
				mouse_pos.y /= SLOT_HEIGHT;
				grabbed_item_offset = (vector3_t){ temp.x - mouse_pos.x * SLOT_WIDTH, temp.y - mouse_pos.y * SLOT_HEIGHT };
				slot = mouse_pos.x;
			}
			else if (rectangle_is_point_inside(armor_inventory, mouse_pos))
			{
				mouse_pos.x -= armor_inventory.x;
				mouse_pos.y -= armor_inventory.y;
				pointi_t temp = mouse_pos;
				mouse_pos.x /= SLOT_WIDTH;
				mouse_pos.y /= SLOT_HEIGHT;
				grabbed_item_offset = (vector3_t){ temp.x - mouse_pos.x * SLOT_WIDTH, temp.y - mouse_pos.y * SLOT_HEIGHT };
				slot = mouse_pos.y + 36;
			}
			
			if (slot != -1)
			{
				block_type_t item = inventory->items[slot];
				if (item == BLOCK_AIR && grabbed_item_index != -1)
				{
					inventory->items[slot] = inventory->items[grabbed_item_index];
					inventory->items[grabbed_item_index] = BLOCK_AIR;
					grabbed_item_index = -1;
				}
				else if (item != BLOCK_AIR)
				{
					if (grabbed_item_index == slot)
					{
						grabbed_item_index = -1;
						return;
					}
					else if (grabbed_item_index != -1)
					{
						block_type_t temp = inventory->items[grabbed_item_index];
						inventory->items[grabbed_item_index] = item;
						inventory->items[slot] = temp;
						return;
					}
					grabbed_item_index = slot;
				}
			}
		}
	}
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
	grabbed_item_index = -1;
	show_inventory = state;
	invalidate = true;
}

void interface_set_inventory(inventory_t* _inventory)
{
	grabbed_item_index = -1;
	inventory = _inventory;
	invalidate = true;
}