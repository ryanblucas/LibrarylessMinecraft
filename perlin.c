/*
	perlin.c ~ RL
	2-D Perlin noise implementation
*/

#include "perlin.h"
#include <math.h>
#include <string.h>

struct perlin_vector2
{
	double x, y;
};

static int p_original[] =
{
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
	8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
	35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
	134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
	55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
	18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
	250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
	189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43,
	172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
	228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
	107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150,
	254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

/* Large permutation table */
static int p_large[512];

static inline void perlin_check_init(void)
{
	if (p_large[0] != p_original[0])
	{
		memcpy(p_large, p_original, sizeof p_original);
		memcpy(p_large + 256, p_original, sizeof p_original);
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

	int xi = (int)x & 0xFF,
		yi = (int)y & 0xFF;
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

double perlin_brownian_at(double x, double y, int count, double amplitude, double frequency)
{
	double res = 0.0;
	for (int i = 0; i < count; i++)
	{
		res += amplitude * perlin_at(frequency * x, frequency * y);

		amplitude *= 0.5;
		frequency *= 2.0;
	}
	return res;
}