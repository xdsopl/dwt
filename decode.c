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

void quantization(float *values, int length, int len, int xoff, int yoff, int quant, int rounding)
{
	for (int y = 0; y < len; ++y) {
		for (int x = 0; x < len; ++x) {
			int idx = length * (yoff + y) + xoff + x;
			float v = values[idx];
			if (rounding) {
				float bias = 0.375f;
				if (v < 0.f)
					v -= bias;
				else if (v > 0.f)
					v += bias;
			}
			v /= quant;
			values[idx] = v;
		}
	}
}

void copy(float *output, float *input, int width, int height, int length, int stride)
{
	int xoff = (length - width) / 2;
	int yoff = (length - height) / 2;
	for (int j = 0; j < height; ++j)
		for (int i = 0; i < width; ++i)
			output[(width*j+i)*stride] = input[length*(yoff+j)+xoff+i];
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s input.dwt output.ppm\n", argv[0]);
		return 1;
	}
	struct bits_reader *bits = bits_reader(argv[1]);
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
	float *input = malloc(sizeof(float) * 3 * pixels);
	for (int len = lmin/2; len <= length/2; len *= 2) {
		if (!get_bit(bits)) {
			int factor = length / len;
			width /= factor;
			height /= factor;
			for (int j = 0; j < 3; ++j)
				for (int y = 0; y < len; ++y)
					for (int x = 0; x < len; ++x)
						input[len*(len*j+y)+x] = input[length*(length*j+y)+x] / factor;
			length = len;
			pixels = length * length;
			break;
		}
		int qadj = get_vli(bits);
		for (int j = 0; j < 3; ++j) {
			if (!quant[j])
				continue;
			float *values = input + pixels * j;
			for (int yoff = 0; yoff < len*2; yoff += len) {
				for (int xoff = (!yoff && len >= lmin) * len; xoff < len*2; xoff += len) {
					for (int i = 0; i < len*len; ++i) {
						struct position pos = hilbert(len, i);
						int idx = length * (yoff + pos.y) + xoff + pos.x;
						int val = get_vli(bits);
						if (val) {
							if (get_bit(bits))
								val = -val;
						} else {
							int cnt = get_vli(bits);
							for (int k = 0; k < cnt; ++k) {
								values[idx] = 0;
								struct position pos = hilbert(len, ++i);
								idx = length * (yoff + pos.y) + xoff + pos.x;
							}
						}
						values[idx] = val;
					}
					quantization(values, length, len, xoff, yoff, quant[j] >> qadj, (xoff || yoff) && rounding);
				}
			}
		}
	}
	close_reader(bits);
	struct image *image = new_image(argv[2], width, height);
	float *output = malloc(sizeof(float) * pixels);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j]) {
			for (int i = 0; i < pixels; ++i)
				image->buffer[3*i+j] = 0;
			continue;
		}
		transformation(output, input + pixels * j, length, lmin, wavelet);
		copy(image->buffer+j, output, width, height, length, 3);
	}
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

