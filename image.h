/*
Image buffer with color space conversion

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <math.h>

struct image {
	float *buffer;
	int width, height, total;
	char *name;
};

void delete_image(struct image *image)
{
	free(image->buffer);
	free(image);
}

struct image *new_image(char *name, int width, int height)
{
	struct image *image = malloc(sizeof(struct image));
	image->height = height;
	image->width = width;
	image->total = width * height;
	image->name = name;
	image->buffer = malloc(3 * sizeof(float) * width * height);
	return image;
}

float fclampf(float x, float a, float b)
{
	return fminf(fmaxf(x, a), b);
}

void ycocg2rgb(float *io)
{
	float Y = io[0];
	float U = io[1];
	float V = io[2];
	float T = Y - floorf(V / 2.f);
	float G = V + T;
	float B = T - floorf(U / 2.f);
	float R = B + U;
	io[0] = fclampf(R, 0.f, 255.f);
	io[1] = fclampf(G, 0.f, 255.f);
	io[2] = fclampf(B, 0.f, 255.f);
}

void rgb2ycocg(float *io)
{
	float R = io[0];
	float G = io[1];
	float B = io[2];
	float U = R - B;
	float T = B + floorf(U / 2.f);
	float V = G - T;
	float Y = T + floorf(V / 2.f);
	io[0] = Y;
	io[1] = U;
	io[2] = V;
}

void ycocg_from_rgb(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rgb2ycocg(image->buffer + 3 * i);
}

void rgb_from_ycocg(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		ycocg2rgb(image->buffer + 3 * i);
}

