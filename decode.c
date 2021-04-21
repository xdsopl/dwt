/*
dwt - playing with dwt and lossy image compression
Written in 2014 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"

void blah(float *output, float *input, int N, int Q)
{
	for (int i = 0; i < N * N; i++)
		output[i * 3] = input[i * 3] / Q;
	for (int i = 0; i < N; i++)
		ihaar(input + 3 * i, output + 3 * i, N, 3 * N);
	for (int i = 0; i < N; i++)
		ihaar(output + 3 * N * i, input + 3 * N * i, N, 3);
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
	int pixels = 3 * length * length;
	int quant[3];
	for (int i = 0; i < 3; ++i)
		quant[i] = get_vli(bits);
	float *input = malloc(sizeof(float) * pixels);
	for (int i = 0; i < pixels; i++) {
		float val = get_vli(bits);
		if (val) {
			if (get_bit(bits))
				val = -val;
		} else {
			int cnt = get_vli(bits);
			for (int k = 0; k < cnt; ++k)
				input[i++] = 0;
		}
		input[i] = val;
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

