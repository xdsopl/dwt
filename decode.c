/*
Decoder for lossy image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"

void blah(float *output, float *input, int N, int Q)
{
	for (int i = 0; i < N * N; i++)
		input[i * 3] /= Q;
	ihaar2(output, input, N, 3);
}

void doit(struct image *output, float *input, int *quant)
{
	int N = output->width;
	blah(output->buffer + 0, input + 0, N, quant[0]);
	blah(output->buffer + 1, input + 1, N, quant[1]);
	blah(output->buffer + 2, input + 2, N, quant[2]);
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
	doit(output, input, quant);
	if (mode)
		rgb_image(output);
	if (!write_ppm(output))
		return 1;
	return 0;
}

