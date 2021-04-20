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

void ycbcr2rgb(float *io)
{
	float WR = 0.2126f;
	float WB = 0.0722f;
	float WG = 1.0f - WR - WB;
	float UMAX = 0.5f;
	float VMAX = 0.5f;
	float y = io[0], u = io[1], v = io[2];
	io[0] = fclampf(y + (1.0f - WR) / VMAX * v, 0.0f, 1.0f);
	io[1] = fclampf(y - WB * (1.0f - WB) / (UMAX * WG) * u - WR * (1.0f - WR) / (VMAX * WG) * v, 0.0f, 1.0f);
	io[2] = fclampf(y + (1.0f - WB) / UMAX * u, 0.0f, 1.0f);
}

void rgb2ycbcr(float *io)
{
	float WR = 0.2126f;
	float WB = 0.0722f;
	float WG = 1.0f - WR - WB;
	float UMAX = 0.5f;
	float VMAX = 0.5f;
	float r = io[0], g = io[1], b = io[2];
	io[0] = fclampf(WR * r + WG * g + WB * b, 0.0f, 1.0f);
	io[1] = fclampf(UMAX / (1.0f - WB) * (b - io[0]), -UMAX, UMAX);
	io[2] = fclampf(VMAX / (1.0f - WR) * (r - io[0]), -VMAX, VMAX);
}

void ycbcr_image(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rgb2ycbcr(image->buffer + 3 * i);
}

void rgb_image(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		ycbcr2rgb(image->buffer + 3 * i);
}

