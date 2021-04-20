/*
dwt - playing with dwt and lossy image compression
Written in 2014 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "dwt.h"

void blah(float *output, float *input, int N, int Q)
{
	for (int i = 0; i < N * N; i++)
		output[i * 3] = input[i * 3] / Q;
	for (int i = 0; i < N; i++)
		ihaar(input + 3 * i, output + 3 * i, N, 3 * N);
	for (int i = 0; i < N; i++)
		ihaar(output + 3 * N * i, input + 3 * N * i, N, 3);
}

void doit(struct image *output, float *input, short *quant)
{
	int N = output->width;
	blah(output->buffer + 0, input + 0, N, quant[0]);
	blah(output->buffer + 1, input + 1, N, quant[1]);
	blah(output->buffer + 2, input + 2, N, quant[2]);
	rgb_image(output);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s input.dwt output.ppm\n", argv[0]);
		return 1;
	}
	FILE *file = fopen(argv[1], "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read.\n", argv[1]);
		return 1;
	}
	short head[4];
	if (fread(head, sizeof(head), 1, file) != 1) {
		fprintf(stderr, "could not read from file \"%s\".\n", argv[1]);
		fclose(file);
		return 1;
	}
	float *input = malloc(sizeof(float) * 3 * head[0] * head[0]);
	for (int i = 0; i < 3 * head[0] * head[0]; i++) {
		short tmp;
		if (fread(&tmp, sizeof(tmp), 1, file) != 1) {
			fprintf(stderr, "could not read from file \"%s\".\n", argv[1]);
			fclose(file);
			return 1;
		}
		input[i] = tmp;
	}
	fclose(file);
	struct image *output = new_image(argv[2], head[0], head[0]);
	doit(output, input, head+1);
	if (!write_ppm(output))
		return 1;
	return 0;
}

