/*
Encoder for lossless image compression based on the discrete wavelet transformation

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#include "haar.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"
#include "hilbert.h"

int pow2(int N)
{
	return !(N & (N - 1));
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 4) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [MODE]\n", argv[0]);
		return 1;
	}
	int mode = 1;
	if (argc == 4)
		mode = atoi(argv[3]);
	int lmin = 8;
	struct image *input = read_ppm(argv[1]);
	if (!input || input->width != input->height || !pow2(input->width) || input->width < lmin)
		return 1;
	int length = input->width;
	int pixels = length * length;
	if (mode)
		rct_image(input);
	int *output = malloc(sizeof(int) * pixels);
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_vli(bits, length);
	put_vli(bits, lmin);
	int zeros = 0;
	for (int j = 0; j < 3; ++j) {
		haar2d(output, input->buffer+j, lmin, length, 1, 3);
		for (int i = 0; i < pixels; ++i) {
			if (output[hilbert(length, i)]) {
				put_vli(bits, abs(output[hilbert(length, i)]));
				put_bit(bits, output[hilbert(length, i)] < 0);
			} else {
				int pos0 = ftell(bits->file) * 8 + bits->cnt;
				put_vli(bits, 0);
				int k = i + 1;
				while (k < pixels && !output[hilbert(length, k)])
					++k;
				--k;
				put_vli(bits, k - i);
				i = k;
				int pos1 = ftell(bits->file) * 8 + bits->cnt;
				zeros += pos1 - pos0;
			}
		}
	}
	fprintf(stderr, "bits used to encode zeros: %d%%\n", (100 * zeros) / (int)(ftell(bits->file) * 8 + bits->cnt));
	close_writer(bits);
	return 0;
}

