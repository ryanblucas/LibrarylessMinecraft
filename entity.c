/*
	entity.c ~ RL
*/

#include "entity.h"
#include "camera.h"
#include "graphics.h"
#include "interface.h"
#include "window.h"
#include "world.h"
#include <stdio.h>
#include <float.h>

struct player_internal
{
	bool noclip_on;
	inventory_t inventory;
};

void entity_player_init(entity_t* ent)
{
	/* Minecraft's player dimensions */
	ent->hitbox = aabb_set_dimensions(ent->hitbox, (vector3_t){ 0.6F, 1.8F, 0.6F });
	ent->reserved = mc_malloc(sizeof(struct player_internal));
	memset(ent->reserved, 0, sizeof(struct player_internal));

	ent->health = PLAYER_HEART_COUNT;
	struct player_internal* internal = (struct player_internal*)ent->reserved;

	internal->inventory.items[0] = BLOCK_GRASS;
	internal->inventory.items[1] = BLOCK_DIRT;
	internal->inventory.items[2] = BLOCK_STONE;
	internal->inventory.items[3] = BLOCK_LOG;

	internal->inventory.items[9] = BLOCK_WATER;
	for (int i = BLOCK_LEAVES, j = 10; i < BLOCK_COUNT && j < 36; i++, j++)
	{
		internal->inventory.items[j] = i;
	}

	interface_set_current_hearts(ent->health);
	interface_set_inventory(&internal->inventory);
}

void entity_player_destroy(entity_t* ent)
{
	free(ent->reserved);
}

static vector3_t entity_player_make_move_vector(vector3_t forward, vector3_t right)
{
	vector3_t desired = { 0 };

	if (interface_is_inventory_open())
	{
		return desired;
	}

	if (window_input_down(INPUT_FORWARD))
	{
		desired = vector3_add(desired, forward);
	}
	else if (window_input_down(INPUT_BACKWARD))
	{
		desired = vector3_sub(desired, forward);
	}
	if (window_input_down(INPUT_LEFT))
	{
		desired = vector3_add(desired, right);
	}
	else if (window_input_down(INPUT_RIGHT))
	{
		desired = vector3_sub(desired, right);
	}
	return vector3_normalize(desired);
}

static void entity_player_move_standard(entity_t* ent, float delta)
{
	vector3_t forward = camera_forward(),
		right = camera_right();
	forward.y = 0.0F;
	forward = vector3_normalize(forward);
	right.y = 0.0F;
	right = vector3_normalize(right);

	vector3_t desired = entity_player_make_move_vector(forward, right);

	float speed = window_input_down(INPUT_SNEAK) ? ENTITY_PLAYER_SNEAK_SPEED : ENTITY_PLAYER_SPEED,
		friction = 1.0F / (1 + delta * ENTITY_DRAG_CONSTANT);
	ent->velocity = vector3_mul(vector3_add(ent->velocity, vector3_mul_scalar(desired, speed * delta)), (vector3_t) { friction, 1.0F, friction });

	if (window_input_clicked(INPUT_JUMP) && ent->grounded)
	{
		ent->velocity.y = 6.0F;
	}

	bool old_grounded = ent->grounded;
	float old_y_velocity = ent->velocity.y;
	entity_gravity_then_move(ent, delta);
	if (ent->grounded && !old_grounded)
	{
		old_y_velocity += 12.0F;
		old_y_velocity = -old_y_velocity * 1.4F;
		if (old_y_velocity >= 1.0F)
		{
			entity_damage(ent, (int)old_y_velocity);
		}
	}
}

static void entity_player_move_noclip(entity_t* ent, float delta)
{
	vector3_t desired = entity_player_make_move_vector(camera_forward(), camera_right());
	float speed = window_input_down(INPUT_SNEAK) ? 30.0F : 15.0F;
	desired = vector3_mul_scalar(desired, delta * speed);
	ent->hitbox = aabb_translate(ent->hitbox, desired);
}

void entity_player_update(entity_t* ent, float delta)
{
	ent->prev_position = aabb_get_center(ent->hitbox);

	if (window_input_down(INPUT_TELEPORT_TO_SPAWN))
	{
		ent->hitbox = aabb_set_center(ent->hitbox, (vector3_t) { 8, 130, 8 });
		ent->velocity.y = 0.0F;
	}

	struct player_internal* internal = ent->reserved;
	if (window_input_clicked(INPUT_TOGGLE_NOCLIP))
	{
		internal->noclip_on = !internal->noclip_on;
	}

	if (internal->noclip_on)
	{
		ent->velocity.y = 0.0F;
		entity_player_move_noclip(ent, delta);
	}
	else
	{
		entity_player_move_standard(ent, delta);
	}

	if (window_input_clicked(INPUT_OPEN_INVENTORY))
	{
		interface_set_inventory_state(!interface_is_inventory_open());
	}

	if (interface_is_inventory_open())
	{
		return;
	}

	if (window_input_clicked(INPUT_BREAK_BLOCK))
	{
		world_block_set(world_ray_cast(camera_position(), camera_forward(), 5.0F, RAY_SOLID).block, BLOCK_AIR);
	}
	if (window_input_clicked(INPUT_PLACE_BLOCK))
	{
		block_coords_t bc = world_ray_neighbor(world_ray_cast(camera_position(), camera_forward(), 5.0F, RAY_SOLID));
		if (!aabb_collides_aabb(ent->hitbox, (aabb_t) { .min = block_coords_to_vector(bc), .max = vector3_add_scalar(block_coords_to_vector(bc), 1.0F) }))
		{
			world_block_set(bc, internal->inventory.items[internal->inventory.active_slot]);
		}
	}

	internal->inventory.active_slot += window_mouse_wheel_delta();
	if (internal->inventory.active_slot < 0)
	{
		internal->inventory.active_slot += 9;
	}
	internal->inventory.active_slot %= 9;
}

bool entity_player_is_noclipping(const entity_t* ent)
{
	struct player_internal* internal = ent->reserved;
	return internal->noclip_on;
}

void entity_damage(entity_t* ent, int dmg)
{
	ent->health -= dmg;
	printf("Entity hurt with %i damage, now at %i health\n", dmg, ent->health);
	/* TEMPORARY */
	interface_set_current_hearts(ent->health);
}

#define MOVEMENT_EPSILON	0.01F
#define MOVEMENT_MAX_LENGTH	0.7F

collision_face_t entity_move(entity_t* ent, vector3_t addend)
{
	if (vector3_magnitude(addend) > MOVEMENT_MAX_LENGTH)
	{
		collision_face_t res = 0;
		int iter_cnt = (int)(vector3_magnitude(addend) / MOVEMENT_MAX_LENGTH + 1.0F);
		addend = vector3_div_scalar(addend, iter_cnt);
		for (int i = 0; i < iter_cnt; i++)
		{
			res |= entity_move(ent, addend);
		}
		return res;
	}

	aabb_t arr[0x100];
	int len = world_region_aabb(
		vector_to_block_coords(vector3_sub_scalar(ent->hitbox.min, 2.0F)),
		vector_to_block_coords(vector3_add_scalar(ent->hitbox.max, 2.0F)),
		arr, sizeof arr / sizeof * arr);

	vector3_uarray_t uvelocity = { addend };
	collision_face_t res = 0;
	for (int i = 0; i < 3; i++)
	{
		aabb_t moved = aabb_translate_axis(ent->hitbox, i, uvelocity.raw[i]);
		for (int j = 0; j < len; j++)
		{
			if (!aabb_collides_aabb(moved, arr[j]))
			{
				continue;
			}

			res |= (uvelocity.raw[i] > 0.0F ? 0b01 : 0b10) << (i * 2);
			float depth = (vector3_uarray_t){ aabb_collision_depth(moved, arr[j]) }.raw[i];
			uvelocity.raw[i] += (uvelocity.raw[i] > 0.0F ? -1.0F : 1.0F) * (depth + MOVEMENT_EPSILON);

			/* prevent jitter from "zero" movement */
			uvelocity.raw[i] = fabsf(uvelocity.raw[i]) <= MOVEMENT_EPSILON ? 0.0F : uvelocity.raw[i];

			moved = aabb_translate_axis(ent->hitbox, i, uvelocity.raw[i]);
		}
		ent->hitbox = moved;
	}

	return res;
}