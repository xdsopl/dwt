/*
Image buffer with color space conversion

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>

struct image {
	int *buffer;
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
	image->buffer = malloc(3 * sizeof(int) * width * height);
	return image;
}

int clamp(int x, int a, int b)
{
	return x < a ? a : x > b ? b : x;
}

void ycocg2rgb(int *io)
{
	int Y = io[0];
	int U = io[1];
	int V = io[2];
	int T = Y - V / 2;
	int G = V + T;
	int B = T - U / 2;
	int R = B + U;
	io[0] = clamp(R, 0, 255);
	io[1] = clamp(G, 0, 255);
	io[2] = clamp(B, 0, 255);
}

void rgb2ycocg(int *io)
{
	int R = io[0];
	int G = io[1];
	int B = io[2];
	int U = R - B;
	int T = B + U / 2;
	int V = G - T;
	int Y = T + V / 2;
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

