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

void quantization(float *values, int length, int len, int xoff, int yoff, int quant, int rounding)
{
	for (int y = 0; y < len; ++y) {
		for (int x = 0; x < len; ++x) {
			int idx = length * (yoff + y) + xoff + x;
			float v = values[idx];
			v *= quant;
			if (rounding)
				v = truncf(v);
			else
				v = nearbyintf(v);
			values[idx] = v;
		}
	}
}

void copy(float *output, float *input, int width, int height, int length, int col, int row, int cols, int rows, int stride)
{
	if (width == length && height == length) {
		for (int i = 0; i < length * length; ++i)
			output[i] = input[i*stride];
		return;
	}
	int xlen = width / cols;
	int ylen = height / rows;
	int xoff = (length - xlen) / 2;
	int yoff = (length - ylen) / 2;
	int w1 = width - 1, h1 = height - 1;
	for (int j = 0, y = ylen*row+2*h1-yoff; j < length; ++j, ++y)
		for (int i = 0, x = xlen*col+2*w1-xoff; i < length; ++i, ++x)
			output[length*j+i] = input[(width*(h1-abs(h1-y%(2*h1)))+w1-abs(w1-x%(2*w1)))*stride];
}

int pow2(int N)
{
	return !(N & (N - 1));
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
	int length = width;
	int cols = 1;
	int rows = 1;
	if (width != height || !pow2(width)) {
		length = lmin;
		while (length < width && length < height)
			length *= 2;
		cols = (width + length - 1) / length;
		rows = (height + length - 1) / length;
		while (cols > 1 && length-width/cols < length/10)
			++cols;
		while (rows > 1 && length-height/rows < length/10)
			++rows;
	}
	int pixels = length * length;
	fprintf(stderr, "%d cols and %d rows of len %d\n", cols, rows, length);
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
	if (mode) {
		ycbcr_image(image);
		for (int i = 0; i < width * height; ++i)
			image->buffer[3*i] -= 0.5f;
	} else {
		for (int i = 0; i < 3 * width * height; ++i)
			image->buffer[i] -= 0.5f;
	}
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * 3 * pixels * rows * cols);
	for (int j = 0; j < 3; ++j) {
		if (!quant[j])
			continue;
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				float *values = output + pixels * ((cols * row + col) * 3 + j);
				copy(input, image->buffer+j, width, height, length, col, row, cols, rows, 3);
				transformation(values, input, length, lmin, wavelet);
			}
		}
	}
	delete_image(image);
	free(input);
	struct bits_writer *bits = bits_writer(argv[2], capacity);
	if (!bits)
		return 1;
	put_bit(bits, mode);
	put_bit(bits, wavelet);
	put_bit(bits, rounding);
	put_vli(bits, width);
	put_vli(bits, height);
	put_vli(bits, length);
	put_vli(bits, lmin);
	put_vli(bits, cols);
	put_vli(bits, rows);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	int qadj_max = 0;
	while ((!quant[0] || quant[0] >> qadj_max) &&
		(!quant[1] || quant[1] >> qadj_max) &&
		(!quant[2] || quant[2] >> qadj_max))
			++qadj_max;
	if (qadj_max > 0)
		--qadj_max;
	int qadj = 0;
	for (int len = lmin/2; len <= length/2; len *= 2) {
		bits_flush(bits);
		put_bit(bits, 1);
		put_vli(bits, qadj);
		for (int j = 0; j < 3; ++j) {
			if (!quant[j])
				continue;
			for (int row = 0; row < rows; ++row) {
				for (int col = 0; col < cols; ++col) {
					float *values = output + pixels * ((cols * row + col) * 3 + j);
					for (int yoff = 0; yoff < len*2; yoff += len) {
						for (int xoff = (!yoff && len >= lmin) * len; xoff < len*2; xoff += len) {
							quantization(values, length, len, xoff, yoff, quant[j] >> qadj, (xoff || yoff) && rounding);
							int last = 0;
							for (int i = 0; i < len*len; ++i) {
								struct position pos = hilbert(len, i);
								int idx = length * (yoff + pos.y) + xoff + pos.x;
								if (values[idx]) {
									if (i - last) {
										put_vli(bits, 0);
										put_vli(bits, i - last - 1);
									}
									last = i + 1;
									put_vli(bits, fabsf(values[idx]));
									put_bit(bits, values[idx] < 0.f);
								}
							}
							if (last < len*len) {
								put_vli(bits, 0);
								put_vli(bits, len*len - last - 1);
							}
						}
					}
				}
			}
		}
		int cnt = bits_count(bits);
		if (cnt >= capacity) {
			bits_discard(bits);
			fprintf(stderr, "%d bits over capacity, discarding %.1f%% of pixels\n", cnt-capacity+1, (100.f*(length*length-len*len))/(length*length));
			break;
		}
		qadj = (cnt * (length / len) - capacity) / capacity;
		qadj = qadj < 0 ? 0 : qadj > qadj_max ? qadj_max : qadj;
		if (qadj && len < length/4)
			fprintf(stderr, "adjusting quantization by %d in len %d\n", qadj, 2*len);
	}
	put_bit(bits, 0);
	fprintf(stderr, "%d bits encoded\n", bits_count(bits));
	close_writer(bits);
	return 0;
}

