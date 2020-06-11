#include "psx.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#define SIZE	196

POLY_F4 f4[6];
u_long rotY,rotZ;

SVECTOR vertices[] = {
	{ -SIZE / 2, -SIZE / 2,	-SIZE / 2,	0},
	{ SIZE / 2,	-SIZE / 2, -SIZE / 2, 0},
	{ SIZE / 2, SIZE / 2, -SIZE / 2, 0},
	{ -SIZE / 2, SIZE / 2, -SIZE / 2, 0},
	{ -SIZE / 2, -SIZE / 2, SIZE / 2, 0},
	{ SIZE / 2, -SIZE / 2, SIZE / 2, 0},
	{ SIZE / 2, SIZE / 2, SIZE / 2, 0},
	{ -SIZE / 2, SIZE / 2, SIZE / 2, 0},
};

int indices[] = {
	0, 1, 2, 3,
	1, 5, 6, 2,
	5, 4, 7, 6,
	4, 0, 3, 7,
	4, 5, 1, 0,
	6, 7, 3, 2,
};

void model_init()
{
	size_t i;
	char colors[6][3] = 
    {
         {255,0,0},
         {0,255,0},
         {0,0,255},
         {255,255,255},
         {255,0,255},
         {100,100,255},
    };
	for (i = 0; i < ARRAY_SIZE(f4); ++i) {
		SetPolyF4(&f4[i]);
		setRGB0(&f4[i], colors[i][0], colors[i][1], colors[i][2]);
	}
}
void model_draw()
{
    POLY_F4 *s = f4;
    int nclip;
	size_t i;
	psGte(0, 0, 500, 0, rotY+=16, rotZ+=16);
	for (i = 0; i < ARRAY_SIZE(indices); i += 4, ++s) {
		nclip = RotAverageNclip4(&vertices[indices[i + 0]],
					 &vertices[indices[i + 1]],
					 &vertices[indices[i + 2]],
					 &vertices[indices[i + 3]],
					 (long *)&s->x0, (long *)&s->x1,
					 (long *)&s->x3, (long *)&s->x2,
					 0, 0, 0);

		if (nclip <= 0)
			continue;

        psAddPrimF4(s);
	}
}
