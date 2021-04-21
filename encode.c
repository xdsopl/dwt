/*
Encoder for lossy image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"
#include "hilbert.h"

void doit(float *output, float *input, int length, int quant)
{
	haar2(output, input, length, 3);
	for (int i = 0; i < length * length; ++i)
		output[i*3] = nearbyintf(quant * output[i*3]);
}

int pow2(int N)
{
	return !(N & (N - 1));
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 6 && argc != 7) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [Q0 Q1 Q2] [MODE]\n", argv[0]);
		return 1;
	}
	struct image *input = read_ppm(argv[1]);
	if (!input || input->width != input->height || !pow2(input->width))
		return 1;
	int mode = 1;
	if (argc == 7)
		mode = atoi(argv[6]);
	int length = input->width;
	int pixels = length * length;
	int quant[3] = { 128, 32, 32 };
	if (argc >= 6)
		for (int i = 0; i < 3; ++i)
			quant[i] = atoi(argv[3+i]);
	float *output = malloc(sizeof(float) * 3 * pixels);
	if (mode)
		ycbcr_image(input);
	for (int i = 0; i < 3; ++i)
		doit(output+i, input->buffer+i, length, quant[i]);
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_vli(bits, length);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < pixels; ++i) {
			if (output[j+3*hilbert(length, i)]) {
				put_vli(bits, fabsf(output[j+3*hilbert(length, i)]));
				put_bit(bits, output[j+3*hilbert(length, i)] < 0.f);
			} else {
				put_vli(bits, 0);
				int k = i + 1;
				while (k < pixels && !output[j+3*hilbert(length, k)])
					++k;
				--k;
				put_vli(bits, k - i);
				i = k;
			}
		}
	}
	close_writer(bits);
	return 0;
}

