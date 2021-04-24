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

void doit(float *output, float *input, int length, int quant, int wavelet)
{
	for (int i = 0; i < length * length; ++i)
		input[i*3] /= quant;
	if (wavelet)
		idwt2d(icdf97, output, input, length, 3);
	else
		ihaar2d(output, input, length, 3);
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
	int length = get_vli(bits);
	int pixels = length * length;
	int quant[3];
	for (int i = 0; i < 3; ++i)
		quant[i] = get_vli(bits);
	float *input = malloc(sizeof(float) * 3 * pixels);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j])
			continue;
		for (int i = 0; i < pixels; ++i) {
			float val = get_vli(bits);
			if (val) {
				if (get_bit(bits))
					val = -val;
			} else {
				int cnt = get_vli(bits);
				for (int k = 0; k < cnt; ++k)
					input[j+3*hilbert(length, i++)] = 0;
			}
			input[j+3*hilbert(length, i)] = val;
		}
	}
	close_reader(bits);
	struct image *output = new_image(argv[2], length, length);
	for (int i = 0; i < 3; ++i)
		if (quant[i])
			doit(output->buffer+i, input+i, length, quant[i], wavelet);
		else
			for (int j = 0; j < pixels; ++j)
				output->buffer[3*j+i] = 0;
	if (mode)
		rgb_image(output);
	if (!write_ppm(output))
		return 1;
	return 0;
}

