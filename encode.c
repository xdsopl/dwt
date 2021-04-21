/*
Encoder for lossy image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"

void blah(float *output, float *input, int N, int Q)
{
	haar2(output, input, N, 3);
	for (int i = 0; i < N * N; i++)
		output[i * 3] = nearbyintf(Q * output[i * 3]);
}

void doit(float *output, struct image *input, int *quant)
{
	int N = input->width;
	blah(output + 0, input->buffer + 0, N, quant[0]);
	blah(output + 1, input->buffer + 1, N, quant[1]);
	blah(output + 2, input->buffer + 2, N, quant[2]);
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
	doit(output, input, quant);
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_vli(bits, length);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < pixels; ++i) {
			if (output[j+3*i]) {
				put_vli(bits, fabsf(output[j+3*i]));
				put_bit(bits, output[j+3*i] < 0.f);
			} else {
				put_vli(bits, 0);
				int k = i + 1;
				while (k < pixels && !output[j+3*k])
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

