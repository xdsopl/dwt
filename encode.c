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
	int xlen = (width + cols - 1) / cols;
	int ylen = (height + rows - 1) / rows;
	int xoff = (length - xlen) / 2;
	int yoff = (length - ylen) / 2;
	int w1 = width - 1, h1 = height - 1;
	for (int j = 0, y = (height*row)/rows+2*h1-yoff; j < length; ++j, ++y)
		for (int i = 0, x = (width*col)/cols+2*w1-xoff; i < length; ++i, ++x)
			output[length*j+i] = input[(width*(h1-abs(h1-y%(2*h1)))+w1-abs(w1-x%(2*w1)))*stride];
}

void encode(struct bits_writer *bits, float *values, int length, int len, int xoff, int yoff)
{
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

void encode_root(struct bits_writer *bits, float *values, int length, int len)
{
	for (int j = 0; j < len; ++j) {
		for (int i = 0; i < len; ++i) {
			float val = values[length*j+i];
			put_vli(bits, fabsf(val));
			if (val)
				put_bit(bits, val < 0.f);
		}
	}
}

int ilog2(int x)
{
	int l = -1;
	for (; x > 0; x /= 2)
		++l;
	return l;
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 6 && argc != 7 && argc != 8) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [Q0 Q1 Q2] [WAVELET] [CAPACITY]\n", argv[0]);
		return 1;
	}
	struct image *image = read_ppm(argv[1]);
	if (!image)
		return 1;
	int width = image->width;
	int height = image->height;
	int dmin = 3;
	int lmin = 1 << dmin;
	int depth = ilog2(width);
	int length = 1 << depth;
	int cols = 1;
	int rows = 1;
	if (width != height || width != length) {
		for (int best = -1, d = dmin, l = lmin; l <= width || l <= height; ++d, l *= 2) {
			int c = (width + l - 1) / l;
			int r = (height + l - 1) / l;
			while (c > 1 && (l-lmin/2)*c < width)
				++c;
			while (r > 1 && (l-lmin/2)*r < height)
				++r;
			if ((width < height && c > 3) || r > 3)
				continue;
			int o = l * l * c * r - width * height;
			if (best < 0 || o < best) {
				best = o;
				cols = c;
				rows = r;
				depth = d;
				length = 1 << d;
			}
		}
	}
	int pixels = length * length;
	fprintf(stderr, "%d cols and %d rows of len %d\n", cols, rows, length);
	int quant[3] = { 128, 32, 32 };
	if (argc >= 6)
		for (int chan = 0; chan < 3; ++chan)
			quant[chan] = atoi(argv[3+chan]);
	for (int chan = 0; chan < 3; ++chan)
		if (!quant[chan])
			return 1;
	int wavelet = 1;
	if (argc >= 7)
		wavelet = atoi(argv[6]);
	int capacity = 1 << 23;
	if (argc >= 8)
		capacity = atoi(argv[7]);
	ycbcr_image(image);
	for (int i = 0; i < width * height; ++i)
		image->buffer[3*i] -= 0.5f;
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * 3 * pixels * rows * cols);
	for (int chan = 0; chan < 3; ++chan) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				float *values = output + pixels * ((cols * row + col) * 3 + chan);
				copy(input, image->buffer+chan, width, height, length, col, row, cols, rows, 3);
				transformation(values, input, length, lmin, wavelet);
			}
		}
	}
	delete_image(image);
	free(input);
	struct bits_writer *bits = bits_writer(argv[2], capacity);
	if (!bits)
		return 1;
	put_bit(bits, wavelet);
	put_vli(bits, width);
	put_vli(bits, height);
	put_vli(bits, depth);
	put_vli(bits, dmin);
	put_vli(bits, cols);
	put_vli(bits, rows);
	for (int chan = 0; chan < 3; ++chan)
		put_vli(bits, quant[chan]);
	fprintf(stderr, "%d bits for meta data\n", bits_count(bits));
	bits_flush(bits);
	for (int chan = 0; chan < 3; ++chan) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				float *values = output + pixels * ((cols * row + col) * 3 + chan);
				quantization(values, length, lmin/2, 0, 0, quant[chan], 0);
				encode_root(bits, values, length, lmin/2);
			}
		}
	}
	fprintf(stderr, "%d bits for root image\n", bits_count(bits));
	int qadj_max = 0;
	while (quant[0] >> qadj_max && quant[1] >> qadj_max && quant[2] >> qadj_max)
		++qadj_max;
	for (int len = lmin/2; len <= length/2; len *= 2) {
		bits_flush(bits);
		put_bit(bits, 1);
		int qadj = (bits_count(bits) * (length / len) - capacity) / capacity;
		qadj = qadj < 0 ? 0 : qadj > qadj_max ? qadj_max : qadj;
		put_vli(bits, qadj);
		if (qadj)
			fprintf(stderr, "adjusting quantization by %d in len %d\n", qadj, len);
		for (int chan = 0; chan < 3; ++chan) {
			if (quant[chan] >> qadj == 0) {
				put_bit(bits, 0);
				continue;
			}
			put_bit(bits, 1);
			for (int row = 0; row < rows; ++row) {
				for (int col = 0; col < cols; ++col) {
					float *values = output + pixels * ((cols * row + col) * 3 + chan);
					for (int yoff = 0; yoff < len*2; yoff += len) {
						for (int xoff = !yoff * len; xoff < len*2; xoff += len) {
							quantization(values, length, len, xoff, yoff, quant[chan] >> qadj, 1);
							encode(bits, values, length, len, xoff, yoff);
						}
					}
				}
			}
		}
		int cnt = bits_count(bits);
		if (cnt >= capacity) {
			bits_discard(bits);
			put_bit(bits, 0);
			fprintf(stderr, "%d bits over capacity, discarding %.1f%% of pixels\n", cnt-capacity+1, (100.f*(length*length-len*len))/(length*length));
			break;
		}
	}
	int cnt = bits_count(bits);
	int bytes = (cnt + 7) / 8;
	int kib = (bytes + 512) / 1024;
	fprintf(stderr, "%d bits (%d KiB) encoded\n", cnt, kib);
	close_writer(bits);
	return 0;
}

