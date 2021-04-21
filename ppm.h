/*
Read and write Netpbm files

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "image.h"

float srgb(float v)
{
	float K0 = 0.04045f;
	float a = 0.055f;
	float phi = 12.92f;
	float gamma = 2.4f;
	return v <= K0 / phi ? v * phi : (1.0f + a) * powf(v, 1.0f / gamma) - a;
}

float linear(float v)
{
	float K0 = 0.04045f;
	float a = 0.055f;
	float phi = 12.92f;
	float gamma = 2.4f;
	return v <= K0 ? v / phi : powf((v + a) / (1.0f + a), gamma);
}

struct image *read_ppm(char *name)
{
	FILE *file = fopen(name, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read.\n", name);
		return 0;
	}
	if ('P' != fgetc(file) || '6' != fgetc(file)) {
		fprintf(stderr, "file \"%s\" not P6 image.\n", name);
		fclose(file);
		return 0;
	}
	int integer[3];
	struct image *image = 0;
	int c = fgetc(file);
	if (EOF == c)
		goto eof;
	for (int i = 0; i < 3; i++) {
		while ('#' == (c = fgetc(file)))
			while ('\n' != (c = fgetc(file)))
				if (EOF == c)
					goto eof;
		while ((c < '0') || ('9' < c))
			if (EOF == (c = fgetc(file)))
				goto eof;
		char str[16];
		for (int n = 0; n < 16; n++) {
			if (('0' <= c) && (c <= '9') && n < 15) {
				str[n] = c;
				if (EOF == (c = fgetc(file)))
					goto eof;
			} else {
				str[n] = 0;
				break;
			}
		}
		integer[i] = atoi(str);
	}
	if (!(integer[0] && integer[1] && integer[2])) {
		fprintf(stderr, "could not read image file \"%s\".\n", name);
		fclose(file);
		return 0;
	}
	if (integer[2] != 255) {
		fprintf(stderr, "cant read \"%s\", only 8 bit per channel SRGB supported at the moment.\n", name);
		fclose(file);
		return 0;
	}
	image = new_image(name, integer[0], integer[1]);
	for (int i = 0; i < 3 * image->total; i++) {
		int v = fgetc(file);
		if (EOF == v)
			goto eof;
		image->buffer[i] = linear(v / 255.0f);
	}
	fclose(file);
	return image;
eof:
	fprintf(stderr, "EOF while reading from \"%s\".\n", name);
	fclose(file);
	delete_image(image);
	return 0;
}

int write_ppm(struct image *image)
{
	FILE *file = fopen(image->name, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to write.\n", image->name);
		return 0;
	}
	if (!fprintf(file, "P6 %d %d 255\n", image->width, image->height)) {
		fprintf(stderr, "could not write to file \"%s\".\n", image->name);
		fclose(file);
		return 0;
	}
	for (int i = 0; i < 3 * image->total; i++) {
		if (EOF == fputc(255.f * srgb(fclampf(image->buffer[i], 0.f, 1.f)), file))
			goto eof;
	}
	fclose(file);
	return 1;
eof:
	fprintf(stderr, "EOF while writing to \"%s\".\n", image->name);
	fclose(file);
	return 0;
}

