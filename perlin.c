/*
	perlin.c ~ RL
	Perlin noise implementation
*/

/* CREDIT: https://github.com/stegu/perlin-noise/blob/master/src/noise1234.c */

#include "perlin.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include <string.h>
#include <time.h>

#define FAST_FLOOR(x) ( ((int)(x)<(x)) ? ((int)x) : ((int)x-1 ) )

struct perlin_state
{
	unsigned int seed;
	int p_large[512];
};

perlin_state_t perlin_create(void)
{
	return perlin_create_with_seed((unsigned int)time(NULL));
}

perlin_state_t perlin_create_with_seed(unsigned int seed)
{
	perlin_state_t res = mc_malloc(sizeof * res);
	res->seed = seed;
	srand(seed);
	for (int i = 0; i < 256; i++)
	{
		int ri = rand() % 256;
		res->p_large[i] = ri;
		res->p_large[ri] = i;
		res->p_large[i + 256] = ri;
		res->p_large[ri + 256] = i;
	}
	return res;
}

unsigned int perlin_get_seed(perlin_state_t state)
{
	return state->seed;
}

void perlin_delete(perlin_state_t* pstate)
{
	free(*pstate);
	*pstate = NULL;
}

static inline double perlin_fade(double t)
{
	return ((6.0 * t - 15.0) * t + 10.0) * t * t * t;
}

static inline double perlin_lerp(double t, double a, double b)
{
	return a + t * (b - a);
}

static double perlin_grad2(int hash, double x, double y)
{
	switch (hash & 0x3)
	{
	case 0x0: return  x + y;
	case 0x1: return -x + y;
	case 0x2: return  -x - y;
	case 0x3: return x - y;
	}
	assert(false);
}

double perlin_at(perlin_state_t state, double x, double y)
{
	int ix0 = FAST_FLOOR(x),	/* Integer part of x */
		iy0 = FAST_FLOOR(y);	/* Integer part of y */
	float fx0 = x - ix0,		/* Fractional part of x */
		fy0 = y - iy0,			/* Fractional part of y */
		fx1 = fx0 - 1.0f,
		fy1 = fy0 - 1.0f;
	int ix1 = (ix0 + 1) & 0xff,  /* Wrap to 0..255 */
		iy1 = (iy0 + 1) & 0xff;
	ix0 = ix0 & 0xff;
	iy0 = iy0 & 0xff;

	double t = perlin_fade(fy0), 
		s = perlin_fade(fx0);
	
	double nx0 = perlin_grad2(state->p_large[ix0 + state->p_large[iy0]], fx0, fy0),
		nx1 = perlin_grad2(state->p_large[ix0 + state->p_large[iy1]], fx0, fy1);
	double n0 = perlin_lerp(t, nx0, nx1);

	nx0 = perlin_grad2(state->p_large[ix1 + state->p_large[iy0]], fx1, fy0);
	nx1 = perlin_grad2(state->p_large[ix1 + state->p_large[iy1]], fx1, fy1);
	double n1 = perlin_lerp(t, nx0, nx1);

	return 0.507 * perlin_lerp(s, n0, n1);
}

double perlin_brownian_at(perlin_state_t state, double x, double y, int count)
{
	double amplitude = 1.0,
		frequency = 0.005;
	double res = 0.0;
	for (int i = 0; i < count; i++)
	{
		res += amplitude * perlin_at(state, frequency * x, frequency * y);

		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return res;
}

static double perlin_grad3(int hash, double x, double y, double z)
{
	switch (hash & 0xF)
	{
	case 0x0: return  x + y;
	case 0x1: return -x + y;
	case 0x2: return  x - y;
	case 0x3: return -x - y;
	case 0x4: return  x + z;
	case 0x5: return -x + z;
	case 0x6: return  x - z;
	case 0x7: return -x - z;
	case 0x8: return  y + z;
	case 0x9: return -y + z;
	case 0xA: return  y - z;
	case 0xB: return -y - z;
	case 0xC: return  y + x;
	case 0xD: return -y + z;
	case 0xE: return  y - x;
	case 0xF: return -y - z;
	}
	assert(false);
}

double perlin_at_3d(perlin_state_t state, double x, double y, double z)
{
	int ix0 = FAST_FLOOR(x),	/* Integer part of x */
		iy0 = FAST_FLOOR(y),	/* Integer part of y */
		iz0 = FAST_FLOOR(z);	/* Integer part of z */
	double fx0 = x - ix0,		/* Fractional part of x */
		fy0 = y - iy0,			/* Fractional part of y */
		fz0 = z - iz0;			/* Fractional part of z */
	double fx1 = fx0 - 1.0f,
		fy1 = fy0 - 1.0f,
		fz1 = fz0 - 1.0f;
	int ix1 = (ix0 + 1) & 0xff,	/* Wrap to 0..255 */
		iy1 = (iy0 + 1) & 0xff,
		iz1 = (iz0 + 1) & 0xff;
	ix0 = ix0 & 0xff;
	iy0 = iy0 & 0xff;
	iz0 = iz0 & 0xff;

	double r = perlin_fade(fz0),
		t = perlin_fade(fy0),
		s = perlin_fade(fx0);
	
	double nxy0 = perlin_grad3(state->p_large[ix0 + state->p_large[iy0 + state->p_large[iz0]]], fx0, fy0, fz0),
		nxy1 = perlin_grad3(state->p_large[ix0 + state->p_large[iy0 + state->p_large[iz1]]], fx0, fy0, fz1);
	double nx0 = perlin_lerp(r, nxy0, nxy1);

	nxy0 = perlin_grad3(state->p_large[ix0 + state->p_large[iy1 + state->p_large[iz0]]], fx0, fy1, fz0);
	nxy1 = perlin_grad3(state->p_large[ix0 + state->p_large[iy1 + state->p_large[iz1]]], fx0, fy1, fz1);
	double nx1 = perlin_lerp(r, nxy0, nxy1);

	double n0 = perlin_lerp(t, nx0, nx1);

	nxy0 = perlin_grad3(state->p_large[ix1 + state->p_large[iy0 + state->p_large[iz0]]], fx1, fy0, fz0);
	nxy1 = perlin_grad3(state->p_large[ix1 + state->p_large[iy0 + state->p_large[iz1]]], fx1, fy0, fz1);
	nx0 = perlin_lerp(r, nxy0, nxy1);

	nxy0 = perlin_grad3(state->p_large[ix1 + state->p_large[iy1 + state->p_large[iz0]]], fx1, fy1, fz0);
	nxy1 = perlin_grad3(state->p_large[ix1 + state->p_large[iy1 + state->p_large[iz1]]], fx1, fy1, fz1);
	nx1 = perlin_lerp(r, nxy0, nxy1);

	double n1 = perlin_lerp(t, nx0, nx1);

	return 0.936 * perlin_lerp(s, n0, n1);
}