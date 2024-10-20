/*
	perlin.h ~ RL
	2-D Perlin noise implementation
*/

#pragma once

/* Returns the perlin noise at (x, y) */
double perlin_at(double x, double y);
/* Returns the "fractal brownian motion" at (x, y). It adds the perlin at (x, y) "count" times,
	multiplying amplitude and frequency by 0.5 and 2.0 respectively on each step. */
double perlin_brownian_at(double x, double y, int count, double amplitude, double frequency);