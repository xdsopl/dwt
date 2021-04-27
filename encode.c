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

void doit(float *output, float *input, int length, int lmin, int quant, int wavelet, int truncate)
{
	if (wavelet)
		dwt2d(cdf97, output, input, lmin, length, 1, 3);
	else
		haar2d(output, input, lmin, length, 1, 3);
	for (int i = 0; i < length * length; ++i) {
		float v = output[i];
		v *= quant;
		if (truncate)
			v = truncf(v);
		else
			v = nearbyintf(v);
		output[i] = v;
	}
}

int pow2(int N)
{
	return !(N & (N - 1));
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 6 && argc != 7 && argc != 8 && argc != 9) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [Q0 Q1 Q2] [MODE] [WAVELET] [TRUNCATE]\n", argv[0]);
		return 1;
	}
	int lmin = 8;
	struct image *input = read_ppm(argv[1]);
	if (!input || input->width != input->height || !pow2(input->width) || input->width < lmin)
		return 1;
	int length = input->width;
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
	float *output = malloc(sizeof(float) * pixels);
	if (mode)
		ycbcr_image(input);
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_bit(bits, wavelet);
	put_bit(bits, truncate);
	put_vli(bits, length);
	put_vli(bits, lmin);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	int zeros = 0;
	for (int j = 0; j < 3; ++j) {
		if (!quant[j])
			continue;
		doit(output, input->buffer+j, length, lmin, quant[j], wavelet, truncate);
		for (int i = 0; i < pixels; ++i) {
			if (output[hilbert(length, i)]) {
				put_vli(bits, fabsf(output[hilbert(length, i)]));
				put_bit(bits, output[hilbert(length, i)] < 0.f);
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

