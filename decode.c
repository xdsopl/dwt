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

void transformation(float *output, float *input, int length, int lmin, int wavelet)
{
	if (wavelet)
		idwt2d(icdf97, output, input, lmin, length, 1, 1);
	else
		ihaar2d(output, input, lmin, length, 1, 1);
}

void quantization(float *output, int *input, int length, int lmin, int quant, int rounding)
{
	for (int j = 0; j < length; ++j) {
		for (int i = 0; i < length; ++i) {
			float v = input[length*j+i];
			if ((i >= lmin/2 || j >= lmin/2) && rounding) {
				float bias = 0.375f;
				if (v < 0.f)
					v -= bias;
				else if (v > 0.f)
					v += bias;
			}
			v /= quant;
			output[length*j+i] = v;
		}
	}
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
	int rounding = get_bit(bits);
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
	int *putput = malloc(sizeof(int) * 3 * pixels);
	struct image *image = new_image(argv[2], width, height);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j])
			continue;
		int *values = putput + pixels * j;
		for (int i = 0; i < pixels; ++i)
			values[i] = 0;
	}
	for (int len = lmin/2; len <= length/2; len *= 2) {
		if (!get_bit(bits))
			break;
		for (int yoff = 0; yoff < len*2; yoff += len) {
			for (int xoff = (!yoff && len > lmin/2) * len; xoff < len*2; xoff += len) {
				int planes[3], pmax = 1;
				for (int j = 0; j < 3; ++j) {
					if (!quant[j])
						continue;
					planes[j] = get_vli(bits);
					if (pmax < planes[j])
						pmax = planes[j];
				}
				for (int plane = pmax-1; plane >= 0; --plane) {
					for (int j = 0; j < 3; ++j) {
						if (!quant[j] || plane >= planes[j])
							continue;
						int *values = putput + pixels * j;
						for (int i = get_vli(bits); i < len*len; i += get_vli(bits) + 1) {
							struct position pos = hilbert(len, i);
							int idx = length * (yoff + pos.y) + xoff + pos.x;
							values[idx] |= 1 << plane;
						}
					}
				}
				for (int j = 0; j < 3; ++j) {
					if (!quant[j])
						continue;
					int *values = putput + pixels * j;
					for (int y = 0; y < len; ++y) {
						for (int x = 0; x < len; ++x) {
							int idx = length * (yoff + y) + xoff + x;
							int mask = 1 << (planes[j]-1);
							if (values[idx] & mask)
								values[idx] ^= ~mask;
						}
					}
				}
			}
		}
	}
	for (int j = 0; j < 3; ++j) {
		if (!quant[j]) {
			for (int i = 0; i < pixels; ++i)
				image->buffer[3*i+j] = 0;
			continue;
		}
		int *values = putput + pixels * j;
		quantization(input, values, length, lmin, quant[j], rounding);
		transformation(output, input, length, lmin, wavelet);
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

