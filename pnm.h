/*
Read and write Netpbm files

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "image.h"

struct image *read_pnm(char *name)
{
	const char *fname = "/dev/stdin";
	if (name[0] != '-' || name[1])
		fname = name;
	FILE *file = fopen(fname, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read.\n", fname);
		return 0;
	}
	int letter = fgetc(file);
	int number = fgetc(file);
	if ('P' != letter || ('5' != number && '6' != number)) {
		fprintf(stderr, "file \"%s\" neither P5 nor P6 image.\n", fname);
		fclose(file);
		return 0;
	}
	int channels = number == '5' ? 1 : 3;
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
		fprintf(stderr, "could not read image file \"%s\".\n", fname);
		fclose(file);
		return 0;
	}
	if (integer[2] != 255) {
		fprintf(stderr, "cant read \"%s\", only 8 bit per channel SRGB supported at the moment.\n", fname);
		fclose(file);
		return 0;
	}
	image = new_image(name, integer[0], integer[1], channels);
	for (int i = 0; i < channels * image->total; i++) {
		int v = fgetc(file);
		if (EOF == v)
			goto eof;
		image->buffer[i] = v;
	}
	fclose(file);
	return image;
eof:
	fprintf(stderr, "EOF while reading from \"%s\".\n", fname);
	fclose(file);
	delete_image(image);
	return 0;
}

int clamp(int x, int a, int b)
{
	return x < a ? a : x > b ? b : x;
}

int write_pnm(struct image *image)
{
	int channels = image->channels;
	assert(channels == 1 || channels == 3);
	const char *fname = "/dev/stdout";
	if (image->name[0] != '-' || image->name[1])
		fname = image->name;
	FILE *file = fopen(fname, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to write.\n", fname);
		return 0;
	}
	int number = channels == 1 ? 5 : 6;
	if (!fprintf(file, "P%d %d %d 255\n", number, image->width, image->height)) {
		fprintf(stderr, "could not write to file \"%s\".\n", fname);
		fclose(file);
		return 0;
	}
	for (int i = 0; i < channels * image->total; i++) {
		if (EOF == fputc(clamp(image->buffer[i], 0, 255), file))
			goto eof;
	}
	fclose(file);
	return 1;
eof:
	fprintf(stderr, "EOF while writing to \"%s\".\n", fname);
	fclose(file);
	return 0;
}

