/*
	perlin.h ~ RL
	Perlin noise implementation
*/

#pragma once

typedef struct perlin_state* perlin_state_t;

/* Creates a perlin state with a random seed */
perlin_state_t perlin_create(void);
/* Creates a perlin state given the seed */
perlin_state_t perlin_create_with_seed(unsigned int seed);
/* Gets seed of state */
unsigned int perlin_get_seed(perlin_state_t state);
/* Deletes the perlin state pointed at by pstate and sets it to NULL. */
void perlin_delete(perlin_state_t* pstate);
/* Returns the perlin noise at (x, y) */
double perlin_at(perlin_state_t state, double x, double y);
/* Returns the "fractal brownian motion" at (x, y). It adds the perlin at (x, y) "count" times,
	multiplying amplitude and frequency by 0.5 and 2.0 respectively on each step. */
double perlin_brownian_at(perlin_state_t state, double x, double y, int count);
/* Returns the perlin noise at (x, y, z) */
double perlin_at_3d(perlin_state_t state, double x, double y, double z);