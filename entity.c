/*
	entity.c ~ RL
*/

#include "entity.h"
#include "camera.h"
#include "graphics.h"
#include "window.h"
#include "world.h"
#include <stdio.h>
#include <float.h>

void entity_player_init(entity_t* ent)
{
	/* Minecraft's player dimensions */
	ent->hitbox = (aabb_t){ .max = { 0.6F, 1.8F, 0.6F } };
	ent->hitbox = aabb_set_center(ent->hitbox, (vector3_t) { 8, 130, 8 });
}

void entity_player_update(entity_t* ent, float delta)
{
	ent->prev_position = aabb_get_center(ent->hitbox);

	if (window_input_down(INPUT_TELEPORT_TO_SPAWN))
	{
		ent->hitbox = aabb_set_center(ent->hitbox, (vector3_t) { 8, 130, 8 });
		ent->velocity.y = 0.0F;
	}

	vector3_t forward = camera_forward(),
		right = camera_right();
	forward.y = 0.0F;
	forward = vector3_normalize(forward);
	right.y = 0.0F;
	right = vector3_normalize(right);

	vector3_t desired = { 0 };
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
	desired = vector3_normalize(desired);

	float speed = window_input_down(INPUT_SNEAK) ? ENTITY_PLAYER_SNEAK_SPEED : ENTITY_PLAYER_SPEED,
		friction = 1.0F / (1 + delta * ENTITY_DRAG_CONSTANT);
	ent->velocity = vector3_mul(vector3_add(ent->velocity, vector3_mul_scalar(desired, speed * delta)), (vector3_t) { friction, 1.0F, friction });

	if (window_input_clicked(INPUT_JUMP) && ent->grounded)
	{
		ent->velocity.y = 6.0F;
	}

	entity_gravity_then_move(ent, delta);

	static block_type_t to_place;
	if (window_input_clicked(INPUT_CYCLE_BLOCK_FORWARD))
	{
		to_place = (to_place + 1) % (BLOCK_COUNT - 1);
	}
	else if (window_input_clicked(INPUT_CYCLE_BLOCK_BACKWARD))
	{
		to_place = (BLOCK_COUNT + to_place - 2) % (BLOCK_COUNT - 1);
	}

	if (window_input_clicked(INPUT_BREAK_BLOCK))
	{
		world_block_set(world_ray_cast(camera_position(), camera_forward(), 5.0F).block, BLOCK_AIR);
	}
	if (window_input_clicked(INPUT_PLACE_BLOCK))
	{
		block_coords_t bc = world_ray_neighbor(world_ray_cast(camera_position(), camera_forward(), 5.0F));
		if (!aabb_collides_aabb(ent->hitbox, (aabb_t) { .min = block_coords_to_vector(bc), .max = vector3_add_scalar(block_coords_to_vector(bc), 1.0F) }))
		{
			world_block_set(bc, to_place + 1);
		}
	}
}

#define MOVEMENT_EPSILON	0.01F

collision_face_t entity_move(entity_t* ent, vector3_t addend)
{
	aabb_t arr[0x100];
	int len = world_region_aabb(
		/* It is crucial that there is a 2x2x2 slack to the hitbox. */
		vector_to_block_coords(vector3_sub_scalar(ent->hitbox.min, 1.0F)),
		vector_to_block_coords(vector3_add_scalar(ent->hitbox.max, 1.0F)),
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