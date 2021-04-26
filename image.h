/*
Image buffer with reversible color transform

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <math.h>

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

void rct2rgb(int *io)
{
	int Y = io[0];
	int U = io[1];
	int V = io[2];
	int G = Y - (U + V + 512) / 4 + 128;
	int R = U + G;
	int B = V + G;
	io[0] = R;
	io[1] = G;
	io[2] = B;
}

void rgb2rct(int *io)
{
	int R = io[0];
	int G = io[1];
	int B = io[2];
	int Y = (R + 2*G + B) / 4;
	int U = R - G;
	int V = B - G;
	io[0] = Y;
	io[1] = U;
	io[2] = V;
}

void rct_image(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rgb2rct(image->buffer + 3 * i);
}

void rgb_image(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rct2rgb(image->buffer + 3 * i);
}

