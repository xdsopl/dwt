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

void quantization(int *output, float *input, int length, int lmin, int quant, int rounding)
{
	for (int j = 0; j < length; ++j) {
		for (int i = 0; i < length; ++i) {
			float v = input[length*j+i];
			v *= quant;
			if ((i >= lmin/2 || j >= lmin/2) && rounding)
				v = truncf(v);
			else
				v = nearbyintf(v);
			output[length*j+i] = v;
		}
	}
}

int count_planes(int *values, int xoff, int yoff, int len, int length)
{
	int neg = -1, pos = 0;
	for (int y = 0; y < len; ++y) {
		for (int x = 0; x < len; ++x) {
			int idx = length * (yoff + y) + xoff + x;
			if (values[idx] < 0)
				neg &= values[idx];
			else
				pos |= values[idx];
		}
	}
	int cnt = sizeof(int) * 8 - 1;
	while (cnt >= 0 && (neg&(1<<cnt)) && !(pos&(1<<cnt)))
		--cnt;
	return cnt + 2;
}

void copy(float *output, float *input, int width, int height, int length, int stride)
{
	int xoff = (length - width) / 2;
	int yoff = (length - height) / 2;
	for (int j = 0; j < length; ++j)
		for (int i = 0; i < length; ++i)
			if (j >= yoff && j < height+yoff && i >= xoff && i < width+xoff)
				output[length*j+i] = input[(width*(j-yoff)+i-xoff)*stride];
			else
				output[length*j+i] = 0;
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 6 && argc != 7 && argc != 8 && argc != 9 && argc != 10) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [Q0 Q1 Q2] [MODE] [WAVELET] [ROUNDING] [CAPACITY]\n", argv[0]);
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
	int rounding = 1;
	if (argc >= 9)
		rounding = atoi(argv[8]);
	int capacity = 1 << 23;
	if (argc >= 10)
		capacity = atoi(argv[9]);
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	int *putput = malloc(sizeof(int) * 3 * pixels);
	if (mode) {
		ycbcr_image(image);
		for (int i = 0; i < width * height; ++i)
			image->buffer[3*i] -= 0.5f;
	} else {
		for (int i = 0; i < 3 * width * height; ++i)
			image->buffer[i] -= 0.5f;
	}
	struct bits_writer *bits = bits_writer(argv[2], capacity);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_bit(bits, wavelet);
	put_bit(bits, rounding);
	put_vli(bits, width);
	put_vli(bits, height);
	put_vli(bits, lmin);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j])
			continue;
		int *values = putput + pixels * j;
		copy(input, image->buffer+j, width, height, length, 3);
		transformation(output, input, length, lmin, wavelet);
		quantization(values, output, length, lmin, quant[j], rounding);
	}
	int skip = 0;
	for (int len = lmin/2; len <= length/2; len *= 2) {
		bits_flush(bits);
		put_bit(bits, 1);
		if (rounding)
			put_vli(bits, skip);
		int pmin = sizeof(int) * 8;
		for (int yoff = 0; yoff < len*2; yoff += len) {
			for (int xoff = (!yoff && len >= lmin) * len; xoff < len*2; xoff += len) {
				int planes[3], pmax = 1;
				for (int j = 0; j < 3; ++j) {
					if (!quant[j])
						continue;
					int *values = putput + pixels * j;
					planes[j] = count_planes(values, xoff, yoff, len, length);
					put_vli(bits, planes[j]);
					if (pmin > planes[j])
						pmin = planes[j];
					if (pmax < planes[j])
						pmax = planes[j];
				}
				for (int plane = pmax-1; plane >= skip; --plane) {
					for (int j = 0; j < 3; ++j) {
						if (!quant[j] || plane >= planes[j])
							continue;
						int *values = putput + pixels * j;
						int mask = 1 << plane;
						int last = 0;
						for (int i = 0; i < len*len; ++i) {
							struct position pos = hilbert(len, i);
							int idx = length * (yoff + pos.y) + xoff + pos.x;
							if (values[idx] & mask) {
								put_vli(bits, i - last);
								last = i + 1;
								if (plane == planes[j]-1)
									values[idx] ^= ~mask;
							}
						}
						put_vli(bits, len*len - last);
					}
				}
			}
		}
		int cnt = bits_count(bits);
		if (cnt >= capacity) {
			bits_discard(bits);
			fprintf(stderr, "%d bits over capacity, discarding %d%% of pixels\n", cnt-capacity+1, (100*(length*length-len*len)) / (length*length));
			break;
		}
		if (rounding) {
			skip = (cnt * (length / len) - capacity) / capacity;
			int skip_max = pmin >= 2 ? pmin-2 : 0;
			skip = skip < 0 ? 0 : skip > skip_max ? skip_max : skip;
			if (skip && len < length/4)
				fprintf(stderr, "skipping %d LSB planes in len %d\n", skip, 2*len);
		}
	}
	put_bit(bits, 0);
	fprintf(stderr, "%d bits encoded\n", bits_count(bits));
	close_writer(bits);
	return 0;
}

