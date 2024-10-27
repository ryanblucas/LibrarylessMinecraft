/*
	entity.h ~ RL
*/

#pragma once

#include "util.h"

#define ENTITY_PLAYER_SPEED			100.0F
#define ENTITY_PLAYER_SNEAK_SPEED	50.0F
#define ENTITY_PLAYER_CAMERA_OFFSET	1.501F

#define ENTITY_DRAG_CONSTANT		10.0F
#define ENTITY_GRAVITY_CONSTANT		2.5F

typedef enum collision_face
{
	FACE_FORWARD =	0b100000,
	FACE_BACKWARD =	0b010000,
	FACE_UP =		0b001000,
	FACE_DOWN =		0b000100,
	FACE_LEFT =		0b000010,
	FACE_RIGHT =	0b000001,
} collision_face_t;

typedef struct entity
{
	vector3_t rotation, velocity;
	aabb_t hitbox;
	bool grounded;
	void* reserved;

	vector3_t prev_position;
} entity_t;

/* Initializes an entity struct to be a player one. */
void entity_player_init(entity_t* ent);
/* Updates an entity using a player update */
void entity_player_update(entity_t* ent, float delta);
/* Is the player noclipping? */
bool entity_player_is_noclipping(const entity_t* ent);

/* Moves an entity given an addend, checking for collisions w/ the world */
collision_face_t entity_move(entity_t* ent, vector3_t addend);

/* Does gravity and moves an entity. Does change an entity's grounded state */
extern inline void entity_gravity_then_move(entity_t* ent, float delta)
{
	collision_face_t face = entity_move(ent, vector3_add(vector3_mul_scalar(ent->velocity, delta), (vector3_t) { 0, -delta, 0 }));
	ent->grounded = face & FACE_UP;
	if (!ent->grounded)
	{
		ent->velocity.y -= delta * 8.8F;
	}
	else
	{
		ent->velocity.y = 0.0F;
	}
}