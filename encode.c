/*
Encoder for lossy image compression based on the discrete wavelet transformation

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
		dwt2d(cdf97, output, input, lmin, length, 1, 1);
	else
		haar2d(output, input, lmin, length, 1, 1);
}

void quantization(int *output, float *input, int length, int lmin, int quant, int truncate)
{
	for (int j = 0; j < length; ++j) {
		for (int i = 0; i < length; ++i) {
			float v = input[length*j+i];
			v *= quant;
			if ((i >= lmin/2 || j >= lmin/2) && truncate)
				v = truncf(v);
			else
				v = nearbyintf(v);
			output[length*j+i] = v;
		}
	}
}

int count_planes(int *putput, int size)
{
	int neg = -1, pos = 0;
	for (int i = 0; i < size; ++i)
		if (putput[i] < 0)
			neg &= putput[i];
		else
			pos |= putput[i];
	int cnt = sizeof(int) * 8 - 1;
	while (cnt >= 0 && (neg&(1<<cnt)) && !(pos&(1<<cnt)))
		--cnt;
	return cnt + 2;
}

void copy(float *output, float *input, int width, int height, int length, int stride)
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
	if (argc != 3 && argc != 6 && argc != 7 && argc != 8 && argc != 9) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [Q0 Q1 Q2] [MODE] [WAVELET] [TRUNCATE]\n", argv[0]);
		return 1;
	}
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
	int quant[3] = { 128, 32, 32 };
	if (argc >= 6)
		for (int i = 0; i < 3; ++i)
			quant[i] = atoi(argv[3+i]);
	int mode = 1;
	if (argc >= 7)
		mode = atoi(argv[6]);
	int wavelet = 1;
	if (argc >= 8)
		wavelet = atoi(argv[7]);
	int truncate = 1;
	if (argc >= 9)
		truncate = atoi(argv[8]);
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	int *putput = malloc(sizeof(int) * pixels);
	if (mode) {
		ycbcr_image(image);
		for (int i = 0; i < width * height; ++i)
			image->buffer[3*i] -= 0.5f;
	} else {
		for (int i = 0; i < 3 * width * height; ++i)
			image->buffer[i] -= 0.5f;
	}
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_bit(bits, wavelet);
	put_bit(bits, truncate);
	put_vli(bits, width);
	put_vli(bits, height);
	put_vli(bits, lmin);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j])
			continue;
		copy(input, image->buffer+j, width, height, length, 3);
		transformation(output, input, length, lmin, wavelet);
		quantization(putput, output, length, lmin, quant[j], truncate);
		int planes = count_planes(putput, pixels);
		put_vli(bits, planes);
		for (int plane = planes-1; plane >= 0; --plane) {
			int mask = 1 << plane;
			int last = 0;
			for (int i = 0; i < pixels; ++i) {
				if (putput[hilbert(length, i)] & mask) {
					put_vli(bits, i - last);
					last = i;
					if (plane == planes-1)
						putput[hilbert(length, i)] ^= ~mask;
				}
			}
			put_vli(bits, pixels - last);
		}
	}
	close_writer(bits);
	return 0;
}

