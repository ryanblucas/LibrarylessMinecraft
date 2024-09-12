/*
	entity.h ~ RL
*/

#pragma once

#include "util.h"

#define ENTITY_PLAYER_SPEED			1.0F
#define ENTITY_PLAYER_SPRINT_SPEED	2.0F
#define ENTITY_DRAG_CONSTANT		0.9F
#define ENTITY_GRAVITY_CONSTANT		2.5F

typedef struct entity
{
	vector3_t rotation, velocity;
	aabb_t hitbox;
} entity_t;

/* Initializes an entity struct to be a player one. */
void entity_player_init(entity_t* ent);
/* Updates an entity using a player update */
void entity_player_update(entity_t* ent, float delta);
/* Moves an entity given velocity, checking for collisions w/ the world */
void entity_move(entity_t* ent, vector3_t velocity);