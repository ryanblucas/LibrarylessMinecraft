/*
	perlin.c ~ RL
	2-D Perlin noise implementation
*/

#include "perlin.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

struct perlin_vector2
{
	double x, y;
};

/* Large permutation table */
static int p_large[512] = { -1 };

static inline void perlin_check_init(void)
{
	if (p_large[0] == -1)
	{
		unsigned int seed = (unsigned int)time(NULL);
		printf("Perlin seed: %u\n", seed);
		srand(seed);
		for (int i = 0; i < 256; i++)
		{
			int ri = rand() % 256;
			p_large[i] = ri;
			p_large[ri] = i;
			p_large[i + 256] = ri;
			p_large[ri + 256] = i;
		}
	}
}

static inline struct perlin_vector2 perlin_constant_vector(int hash)
{
	switch (hash & 3)
	{
	case 0: return (struct perlin_vector2) { 1.0, 1.0 };
	case 1: return (struct perlin_vector2) { -1.0, 1.0 };
	case 2: return (struct perlin_vector2) { -1.0, -1.0 };
	case 3: return (struct perlin_vector2) { 1.0, -1.0 };
	}
	assert(0);
}

static inline double perlin_dot(struct perlin_vector2 a, struct perlin_vector2 b)
{
	return a.x * b.x + a.y * b.y;
}

static inline double perlin_fade(double t)
{
	return ((6.0 * t - 15.0) * t + 10.0) * t * t * t;
}

static inline double perlin_lerp(double t, double a, double b)
{
	return a + t * (b - a);
}

double perlin_at(double x, double y)
{
	perlin_check_init();

	double xd = x - floor(x),
		yd = y - floor(y);
	struct perlin_vector2 v_tl = { xd, yd - 1.0 },
		v_tr = { xd - 1.0, yd - 1.0 },
		v_bl = { xd, yd },
		v_br = { xd - 1.0, yd };

	int xi = (int)floor(x) & 0xFF,
		yi = (int)floor(y) & 0xFF;
	int h_tl = p_large[p_large[xi] + yi + 1],
		h_tr = p_large[p_large[xi + 1] + yi + 1],
		h_bl = p_large[p_large[xi] + yi],
		h_br = p_large[p_large[xi + 1] + yi];

	double dot_tl = perlin_dot(v_tl, perlin_constant_vector(h_tl)),
		dot_tr = perlin_dot(v_tr, perlin_constant_vector(h_tr)),
		dot_bl = perlin_dot(v_bl, perlin_constant_vector(h_bl)),
		dot_br = perlin_dot(v_br, perlin_constant_vector(h_br));

	double u = perlin_fade(xd),
		v = perlin_fade(yd);

	return perlin_lerp(u, perlin_lerp(v, dot_bl, dot_tl), perlin_lerp(v, dot_br, dot_tr));
}

double perlin_brownian_at(double x, double y, int count)
{
	double amplitude = 1.0,
		frequency = 0.005;
	double res = 0.0;
	for (int i = 0; i < count; i++)
	{
		res += amplitude * perlin_at(frequency * x, frequency * y);

		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return res;
}