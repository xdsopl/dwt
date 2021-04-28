/*
Decoder for lossy image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "haar.h"
#include "cdf97.h"
#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"
#include "hilbert.h"

void doit(float *output, float *input, int length, int lmin, int quant, int wavelet, int truncate)
{
	for (int j = 0; j < length; ++j) {
		for (int i = 0; i < length; ++i) {
			float v = input[length*j+i];
			if ((i >= lmin/2 || j >= lmin/2) && truncate) {
				float bias = 0.375f;
				if (v < 0.f)
					v -= bias;
				else if (v > 0.f)
					v += bias;
			}
			v /= quant;
			input[length*j+i] = v;
		}
	}
	if (wavelet)
		idwt2d(icdf97, output, input, lmin, length, 1, 1);
	else
		ihaar2d(output, input, lmin, length, 1, 1);
}

void copy(float *output, float *input, int width, int height, int length, int stride)
{
	for (int j = 0; j < height; ++j)
		for (int i = 0; i < width; ++i)
			output[(width*j+i)*stride] = input[length*j+i];
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s input.dwt output.ppm\n", argv[0]);
		return 1;
	}
	struct bits *bits = bits_reader(argv[1]);
	if (!bits)
		return 1;
	int mode = get_bit(bits);
	int wavelet = get_bit(bits);
	int truncate = get_bit(bits);
	int width = get_vli(bits);
	int height = get_vli(bits);
	int lmin = get_vli(bits);
	int quant[3];
	for (int i = 0; i < 3; ++i)
		quant[i] = get_vli(bits);
	int length = lmin;
	while (length < width || length < height)
		length *= 2;
	int pixels = length * length;
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	struct image *image = new_image(argv[2], width, height);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j]) {
			for (int i = 0; i < pixels; ++i)
				image->buffer[3*i+j] = 0;
			continue;
		}
		for (int i = 0; i < pixels; ++i) {
			float val = get_vli(bits);
			if (val) {
				if (get_bit(bits))
					val = -val;
			} else {
				int cnt = get_vli(bits);
				for (int k = 0; k < cnt; ++k)
					input[hilbert(length, i++)] = 0;
			}
			input[hilbert(length, i)] = val;
		}
		doit(output, input, length, lmin, quant[j], wavelet, truncate);
		copy(image->buffer+j, output, width, height, length, 3);
	}
	close_reader(bits);
	if (mode) {
		for (int i = 0; i < width * height; ++i)
			image->buffer[3*i] += 0.5f;
		rgb_image(image);
	} else {
		for (int i = 0; i < 3 * width * height; ++i)
			image->buffer[i] += 0.5f;
	}
	if (!write_ppm(image))
		return 1;
	return 0;
}

