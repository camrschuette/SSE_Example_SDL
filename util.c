#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_draw.h"
#include "util.h"

int rand_int(int a, int b)
{
	return rand() % (b - a + 1) + a;
}

void init_circles_aos(Circle *circles, int num)
{
	int i;
	for (i = 0; i < num; ++i)
	{
		circles[i].radius = rand_int(5, 15);
		circles[i].x = rand_int(circles[i].radius, 800 - circles[i].radius);
		circles[i].y = rand_int(circles[i].radius, 800 - circles[i].radius);
		
		circles[i].vx = rand_int(-1, 1);
		circles[i].vy = rand_int(-1, 1);
		
		float magnitude = 800.0f / 1000.0f;
		float length = sqrt(circles[i].vx * circles[i].vx + circles[i].vy * circles[i].vy);
		if (length != 0)
		{
			circles[i].vx /= length;
			circles[i].vx *= magnitude;
			
			circles[i].vy /= length;
			circles[i].vy *= magnitude;
		}
	}
}

void draw_circles_aos(SDL_Surface *surface, Circle *circles, int num)
{
	int i;
	for (i = 0; i < num; ++i)
	{
		Draw_FillCircle(surface, circles[i].x, circles[i].y, circles[i].radius, SDL_MapRGB(surface->format, 0, 0, 255));
	}
}

void update_circles_aos(Circle *circles, int num, int dtime)
{
	int i;	
	for (i = 0; i < num; ++i)
	{
		circles[i].x += circles[i].vx * dtime;
		if (circles[i].x <= circles[i].radius)
		{
			circles[i].x = circles[i].radius;
			circles[i].vx *= -1;
		}
		else if (circles[i].x >= 800 - circles[i].radius)
		{
			circles[i].x = 800 - circles[i].radius;
			circles[i].vx *= -1;
		}
		
		circles[i].y += circles[i].vy * dtime;
		if (circles[i].y <= circles[i].radius)
		{
			circles[i].y = circles[i].radius;
			circles[i].vy *= -1;
		}
		else if (circles[i].y >= 800 - circles[i].radius)
		{
			circles[i].y = 800 - circles[i].radius;
			circles[i].vy *= -1;
		}
	}
	
	for (i = 0; i < num; ++i)
	{
		int j;
		for (j = i + 1; j < num - 1; ++j)
		{
			float dx = circles[i].x - circles[j].x;
			float dy = circles[i].y - circles[j].y;
			float dist2 = dx * dx + dy * dy;
			float rsum = circles[i].radius + circles[j].radius;
			
			if (dist2 <= rsum * rsum)
			{
				circles[i].vx *= -1;
				circles[i].vy *= -1;
				
				circles[j].vx *= -1;
				circles[j].vy *= -1;
			}
		}
	}
}

void init_circles_soa(CirclePool *pool, int num)
{
	pool->x = (float *)_aligned_malloc(num * sizeof(float), 16);
	pool->y = (float *)_aligned_malloc(num * sizeof(float), 16);
	pool->vx = (float *)_aligned_malloc(num * sizeof(float), 16);
	pool->vy = (float *)_aligned_malloc(num * sizeof(float), 16);
	pool->radius = (float *)_aligned_malloc(num * sizeof(float), 16);
	
	int i;
	for (i = 0; i < num; ++i)
	{
		pool->radius[i] = rand_int(5, 15);
		pool->x[i] = rand_int(pool->radius[i], 800 - pool->radius[i]);
		pool->y[i] = rand_int(pool->radius[i], 800 - pool->radius[i]);
		
		pool->vx[i] = rand_int(-1, 1);
		pool->vy[i] = rand_int(-1, 1);
		
		float magnitude = 800.0f / 1000.0f;
		float length = sqrt(pool->vx[i] * pool->vx[i] + pool->vy[i] * pool->vy[i]);
		if (length != 0)
		{
			pool->vx[i] /= length;
			pool->vx[i] *= magnitude;
			
			pool->vy[i] /= length;
			pool->vy[i] *= magnitude;
		}
	}
}

void draw_circles_soa(SDL_Surface *surface, CirclePool *pool, int num)
{
	int i;
	for (i = 0; i < num; ++i)
	{
		Draw_FillCircle(surface, pool->x[i], pool->y[i], pool->radius[i], SDL_MapRGB(surface->format, 0, 0, 255));
	}
}

void update_circles_soa(CirclePool *circles, int num, float dtime)
{
	dtime = dtime * 0.1f;

	float tmp[4] __attribute__((aligned(16))) = {0.0f, 0.0f, 0.0f, 0.0f};
	
	static float win_width[4] __attribute__((aligned(16))) = {800.0f, 800.0f, 800.0f, 800.0f};
	const int F_HBIT_ARRAY[4] __attribute__((aligned(16))) = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
	float one[4] __attribute__((aligned(16))) = {1.0f, 1.0f, 1.0f, 1.0f};
	float neg[4] __attribute__((aligned(16))) = {-1.0f, -1.0f, -1.0f, -1.0f};
	
	int i;
	int j;
	for(i = 0; i < num; i += 4)
	{
		for(j = i + 4; j < num; j += 4)
		{
			int x;
			for (x = 0; x < 4; ++x)
			{
				asm("movl (%[circles]), %%eax" "\n\t"			// eax = circles->x
					"movaps (%%eax, %[i], 4), %%xmm0" "\n\t"	// xmm0 = [circles->x[i], .., circles->x[i + 3]]
					
					"movaps %%xmm0, %%xmm1" "\n\t"				// shuffle x
					"shufps $0x93, %%xmm0, %%xmm0" "\n\t"
					"movaps %%xmm0, (%%eax, %[i], 4)" "\n\t"
					"movaps %%xmm1, %%xmm0" "\n\t"
					
					"movaps (%%eax, %[j], 4), %%xmm1" "\n\t"	// xmm1 = [circles->x[j], .., circles->x[j + 3]]
					
					"subps %%xmm1, %%xmm0" "\n\t"
					"mulps %%xmm0, %%xmm0" "\n\t"
					
					"movl 4(%[circles]), %%eax" "\n\t"			// eax = circles->y
					"movaps (%%eax, %[i], 4), %%xmm1" "\n\t"	// xmm1 = [circles->y[i], .., circles->y[i + 3]]
					
					"movaps %%xmm1, %%xmm2" "\n\t"				// shuffle y
					"shufps $0x93, %%xmm1, %%xmm1" "\n\t"
					"movaps %%xmm1, (%%eax, %[i], 4)" "\n\t"
					"movaps %%xmm2, %%xmm1" "\n\t"
					
					"movaps (%%eax, %[j], 4), %%xmm2" "\n\t"	// xmm2 = [circles->y[j], .., circles->y[j + 3]]
					
					"subps %%xmm2, %%xmm1" "\n\t"
					"mulps %%xmm1, %%xmm1" "\n\t"
					
					"addps %%xmm1, %%xmm0" "\n\t"				// dist**2
					
					"movl 16(%[circles]), %%eax" "\n\t"			// eax = circles->radius
					"movaps (%%eax, %[i], 4), %%xmm1" "\n\t"	// xmm1 = [circles->radius[i], .., circles->radius[i + 3]]
					
					"movaps %%xmm1, %%xmm2" "\n\t"				// shuffle radius
					"shufps $0x93, %%xmm1, %%xmm1" "\n\t"
					"movaps %%xmm1, (%%eax, %[i], 4)" "\n\t"
					"movaps %%xmm2, %%xmm1" "\n\t"
					
					"movaps (%%eax, %[j], 4), %%xmm2" "\n\t"	// xmm2 = [circles->radius[j], .., circles->radius[j + 3]]
					
					"addps %%xmm2, %%xmm1" "\n\t"
					"mulps %%xmm1, %%xmm1" "\n\t"				// radius**2
					
					"cmpleps %%xmm1, %%xmm0" "\n\t"				// dist**2 <= radius**2
					"movaps (%[F_HBIT_ARRAY]), %%xmm1" "\n\t"
					"andps %%xmm1, %%xmm0" "\n\t"
					
					"movl 8(%[circles]), %%eax" "\n\t"			// eax = circles->vx
					
					"movaps (%%eax, %[i], 4), %%xmm1" "\n\t"	// xmm1 = [circles->vx[i], .., circles->vx[i + 3]]
					"xorps %%xmm0, %%xmm1" "\n\t"
					"shufps $0x93, %%xmm1, %%xmm1" "\n\t"		// shuffle vx
					"movaps %%xmm1, (%%eax, %[i], 4)" "\n\t"
					
					"movaps (%%eax, %[j], 4), %%xmm2" "\n\t"	// xmm2 = [circles->vx[j], .., circles->vx[j + 3]]
					"xorps %%xmm0, %%xmm2" "\n\t"
					"movaps %%xmm2, (%%eax, %[j], 4)" "\n\t"
					
					"movl 12(%[circles]), %%eax" "\n\t"			// eax = circles->vy
					
					"movaps (%%eax, %[i], 4), %%xmm1" "\n\t"	// xmm1 = [circles->vy[i], .., circles->vy[i + 3]]
					"xorps %%xmm0, %%xmm1" "\n\t"
					"shufps $0x93, %%xmm1, %%xmm1" "\n\t"		// shuffle vy
					"movaps %%xmm1, (%%eax, %[i], 4)" "\n\t"
					
					"movaps (%%eax, %[j], 4), %%xmm2" "\n\t"	// xmm2 = [circles->vy[j], .., circles->vy[j + 3]]
					"xorps %%xmm0, %%xmm2" "\n\t"
					"movaps %%xmm2, (%%eax, %[j], 4)" "\n\t"
					:
					: [circles] "r" (circles), [i] "r" (i), [j] "r" (j),\
					  [F_HBIT_ARRAY] "r" (F_HBIT_ARRAY)
					: "eax", "xmm0", "xmm1", "xmm2");
			}
		}
	}
	
	for (i = 0; i < num; i += 4)
	{
		//This is only for the x axis walls
		asm("movaps %[x], %%xmm0" "\n\t"
			"movaps %[vx], %%xmm1" "\n\t"
			"movaps %[win_width], %%xmm2" "\n\t"
			"movaps %[radius], %%xmm4" "\n\t"
			
			"movss %[dtime], %%xmm3" "\n\t"
			"shufps $0, %%xmm3, %%xmm3" "\n\t"
			
			"mulps %%xmm3, %%xmm1" "\n\t"
			
			"addps %%xmm1, %%xmm0" "\n\t"

			"subps %%xmm4, %%xmm2" "\n\t"
			
			"minps %%xmm2, %%xmm0" "\n\t"
			"maxps %%xmm4, %%xmm0" "\n\t"
			
			
			"movaps %[win_width], %%xmm2" "\n\t"
			"subps %%xmm4, %%xmm2" "\n\t"
			"movaps %[radius], %%xmm4" "\n\t"
			"cmpeqps %%xmm0, %%xmm2" "\n\t"
			"cmpeqps %%xmm0, %%xmm4" "\n\t"
			
			"orps %%xmm4, %%xmm2" "\n\t"
			
			"movaps %[one], %%xmm4" "\n\t"
			"andps %%xmm4, %%xmm2" "\n\t"
			
			"movaps %[neg], %%xmm4" "\n\t"
			"addps %%xmm4, %%xmm2" "\n\t"
			"andnps %%xmm4, %%xmm2" "\n\t"
			
			"movaps %[one], %%xmm4" "\n\t"
			"orps %%xmm4, %%xmm2" "\n\t"
			"movaps %[vx], %%xmm1" "\n\t"
			"mulps %%xmm2, %%xmm1" "\n\t"
			
			"movaps %%xmm1, %[vx]" "\n\t"
			"movaps %%xmm0, %[x]" "\n\t"
			"movaps %%xmm2, %[tmp]" "\n\t"
			: [x] "+m" (*(circles->x + i)), [y] "+m" (*(circles->y + i)), [vx] "+m" (*(circles->vx + i)), [vy] "+m" (*(circles->vy + i)), [tmp] "+m" (*tmp)
			: [dtime] "irm" (dtime), [radius] "m" (*(circles->radius + i)), [win_width] "m" (*win_width), [F_HBIT_ARRAY] "m" (*F_HBIT_ARRAY), [one] "m" (*one), [neg] "m" (*neg) 
			: "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
			);
		
		//this is y axis walls
		asm("movaps %[y], %%xmm0" "\n\t"
			"movaps %[vy], %%xmm1" "\n\t"
			"movaps %[win_width], %%xmm2" "\n\t"
			"movaps %[radius], %%xmm4" "\n\t"
			
			"movss %[dtime], %%xmm3" "\n\t"
			"shufps $0, %%xmm3, %%xmm3" "\n\t"
			
			"mulps %%xmm3, %%xmm1" "\n\t"
			
			"addps %%xmm1, %%xmm0" "\n\t"

			"subps %%xmm4, %%xmm2" "\n\t"
			
			"minps %%xmm2, %%xmm0" "\n\t"
			"maxps %%xmm4, %%xmm0" "\n\t"
			
			
			"movaps %[win_width], %%xmm2" "\n\t"
			"subps %%xmm4, %%xmm2" "\n\t"
			"movaps %[radius], %%xmm4" "\n\t"
			"cmpeqps %%xmm0, %%xmm2" "\n\t"
			"cmpeqps %%xmm0, %%xmm4" "\n\t"
			
			"orps %%xmm4, %%xmm2" "\n\t"
			"movaps %[one], %%xmm4" "\n\t"
			"andps %%xmm4, %%xmm2" "\n\t"
			
			"movaps %[neg], %%xmm4" "\n\t"
			"addps %%xmm4, %%xmm2" "\n\t"
			"andnps %%xmm4, %%xmm2" "\n\t"
			
			"movaps %[one], %%xmm4" "\n\t"
			"orps %%xmm4, %%xmm2" "\n\t"
			"movaps %[vy], %%xmm1" "\n\t"
			"mulps %%xmm2, %%xmm1" "\n\t"
			
			"movaps %%xmm1, %[vy]" "\n\t"
			"movaps %%xmm0, %[y]" "\n\t"
			"movaps %%xmm2, %[tmp]" "\n\t"
			: [x] "+m" (*(circles->x + i)), [y] "+m" (*(circles->y + i)), [vx] "+m" (*(circles->vx + i)), [vy] "+m" (*(circles->vy + i)), [tmp] "+m" (*tmp)
			: [dtime] "irm" (dtime), [radius] "m" (*(circles->radius + i)), [win_width] "m" (*win_width), [F_HBIT_ARRAY] "m" (*F_HBIT_ARRAY), [one] "m" (*one), [neg] "m" (*neg) 
			: "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
			);
			
			//printf("%f, %f, %f, %f\n", tmp[0], tmp[1], tmp[2], tmp[3]);
	}
}
