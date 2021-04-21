/*
Decoder for lossy image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"

void doit(float *output, float *input, int length, int quant)
{
	for (int i = 0; i < length * length; ++i)
		input[i*3] /= quant;
	ihaar2(output, input, length, 3);
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
	int length = get_vli(bits);
	int pixels = length * length;
	int quant[3];
	for (int i = 0; i < 3; ++i)
		quant[i] = get_vli(bits);
	float *input = malloc(sizeof(float) * 3 * pixels);
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < pixels; ++i) {
			float val = get_vli(bits);
			if (val) {
				if (get_bit(bits))
					val = -val;
			} else {
				int cnt = get_vli(bits);
				for (int k = 0; k < cnt; ++k)
					input[j+3*i++] = 0;
			}
			input[j+3*i] = val;
		}
	}
	close_reader(bits);
	struct image *output = new_image(argv[2], length, length);
	for (int i = 0; i < 3; ++i)
		doit(output->buffer+i, input+i, length, quant[i]);
	if (mode)
		rgb_image(output);
	if (!write_ppm(output))
		return 1;
	return 0;
}

