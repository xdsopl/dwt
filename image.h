/*
Image buffer

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
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

