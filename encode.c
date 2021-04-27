/*
Encoder for lossless image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "haar.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"
#include "hilbert.h"


void copy(int *output, int *input, int width, int height, int length, int stride)
{
	for (int j = 0; j < length; ++j)
		for (int i = 0; i < length; ++i)
			if (j < height && i < width)
				output[length*j+i] = input[(width*j+i)*stride];
			else
				output[length*j+i] = 0;
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 4) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [MODE]\n", argv[0]);
		return 1;
	}
	int mode = 1;
	if (argc == 4)
		mode = atoi(argv[3]);
	struct image *image = read_ppm(argv[1]);
	if (!image)
		return 1;
	int width = image->width;
	int height = image->height;
	int lmin = 8;
	int length = lmin;
	while (length < width || length < height)
		length *= 2;
	int pixels = length * length;
	if (mode)
		rct_image(image);
	int *input = malloc(sizeof(int) * pixels);
	int *output = malloc(sizeof(int) * pixels);
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_vli(bits, width);
	put_vli(bits, height);
	put_vli(bits, lmin);
	int zeros = 0;
	for (int j = 0; j < 3; ++j) {
		copy(input, image->buffer+j, width, height, length, 3);
		haar2d(output, input, lmin, length, 1, 1);
		for (int i = 0; i < pixels; ++i) {
			if (output[hilbert(length, i)]) {
				put_vli(bits, abs(output[hilbert(length, i)]));
				put_bit(bits, output[hilbert(length, i)] < 0);
			} else {
				int pos0 = ftell(bits->file) * 8 + bits->cnt;
				put_vli(bits, 0);
				int k = i + 1;
				while (k < pixels && !output[hilbert(length, k)])
					++k;
				--k;
				put_vli(bits, k - i);
				i = k;
				int pos1 = ftell(bits->file) * 8 + bits->cnt;
				zeros += pos1 - pos0;
			}
		}
	}
	fprintf(stderr, "bits used to encode zeros: %d%%\n", (100 * zeros) / (int)(ftell(bits->file) * 8 + bits->cnt));
	close_writer(bits);
	return 0;
}

