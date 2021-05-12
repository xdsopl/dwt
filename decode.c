/*
Decoder for lossy image compression based on the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
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
		idwt2d(icdf97, output, input, lmin, length, 1, 1);
	else
		ihaar2d(output, input, lmin, length, 1, 1);
}

void quantization(float *output, int *input, int *planes, int *missing, int length, int lmin, int quant, int col, int row, int cols, int rows, int chan, int chans)
{
	for (int y = 0, *in = input+(lmin/2)*(lmin/2)*(cols*(rows*chan+row)+col); y < lmin/2; ++y) {
		for (int x = 0; x < lmin/2; ++x) {
			float v = *in++;
			v /= 1 << quant;
			output[length*y+x] = v;
		}
	}
	input += (lmin/2) * (lmin/2) * cols * rows * chans;
	for (int len = lmin/2; len <= length/2; input += 3*len*len*cols*rows*chans, len *= 2, planes += chans, missing += chans) {
		for (int yoff = 0, *in = input+3*len*len*(cols*(rows*chan+row)+col); yoff < len*2; yoff += len) {
			for (int xoff = !yoff * len; xoff < len*2; xoff += len) {
				for (int i = 0; i < len*len; ++i) {
					float v = *in++;
					float bias = 0.375f;
					bias *= 1 << *missing;
					if (v < 0.f)
						v -= bias;
					else if (v > 0.f)
						v += bias;
					v /= 1 << quant;
					if (*planes <= 0)
						v = 0;
					struct position pos = hilbert(len, i);
					output[length*(yoff+pos.y)+xoff+pos.x] = v;
				}
			}
		}
	}
}

float flerpf(float a, float b, float x)
{
	return (1.f - x) * a + x * b;
}

void copy(float *output, float *input, int width, int height, int length, int col, int row, int cols, int rows, int stride)
{
	if (width == length && height == length) {
		for (int i = 0; i < length * length; ++i)
			output[i*stride] = input[i];
		return;
	}
	int xlen = (width + cols - 1) / cols;
	int ylen = (height + rows - 1) / rows;
	int xoff = (length - xlen) / 2;
	int yoff = (length - ylen) / 2;
	for (int j = !row*yoff, y = (height*row)/rows-!!row*yoff; j < length && y < height; ++j, ++y)
		for (int i = !col*xoff, x = (width*col)/cols-!!col*xoff; i < length && x < width; ++i, ++x)
			if ((!col || i >= 2*xoff) && (!row || j >= 2*yoff))
				output[(width*y+x)*stride] = input[length*j+i];
			else
				output[(width*y+x)*stride] = flerpf(output[(width*y+x)*stride], input[length*j+i], fclampf(i/(2.f*xoff), 0.f, 1.f) * fclampf(j/(2.f*yoff), 0.f, 1.f));
}

void decode(struct bits_reader *bits, int *val, int num, int plane)
{
	for (int i = get_vli(bits); i < num; i += get_vli(bits) + 1)
		val[i] |= 1 << plane;
}

void finalize(int *val, int num, int planes)
{
	int mask = 1 << (planes - 1);
	for (int i = 0; i < num; ++i)
		if (val[i] & mask)
			val[i] ^= ~mask;
}

void decode_root(struct bits_reader *bits, int *val, int num)
{
	int cnt = get_vli(bits);
	for (int i = 0; cnt && i < num; ++i) {
		read_bits(bits, val+i, cnt);
		if (val[i] && get_bit(bits))
			val[i] = -val[i];
	}
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s input.dwt output.ppm\n", argv[0]);
		return 1;
	}
	struct bits_reader *bits = bits_reader(argv[1]);
	if (!bits)
		return 1;
	int wavelet = get_bit(bits);
	int width = get_vli(bits);
	int height = get_vli(bits);
	int depth = get_vli(bits);
	int dmin = get_vli(bits);
	int cols = get_vli(bits);
	int rows = get_vli(bits);
	int quant[3];
	for (int chan = 0; chan < 3; ++chan)
		quant[chan] = get_vli(bits);
	int length = 1 << depth;
	int lmin = 1 << dmin;
	int pixels = length * length;
	int *buffer = malloc(sizeof(int) * 3 * pixels * rows * cols);
	for (int i = 0; i < 3 * pixels * rows * cols; ++i)
		buffer[i] = 0;
	for (int chan = 0, num = (lmin/2)*(lmin/2)*cols*rows; chan < 3; ++chan)
		decode_root(bits, buffer+num*chan, num);
	int layers_max = 24;
	int planes[3*layers_max];
	for (int i = 0; i < 3*layers_max; ++i)
		planes[i] = -1;
	int missing[3*layers_max];
	for (int i = 0; i < 3*layers_max; ++i)
		missing[i] = 0;
	for (int layers = 1; layers < layers_max; ++layers) {
		for (int len = lmin/2, num = len*len*cols*rows*3, *buf = buffer+num, layer = 0;
		len <= length/2 && layer < layers; len *= 2, num = len*len*cols*rows*3, ++layer) {
			for (int chan = 0; chan < 3; ++chan, buf += num) {
				if (!get_bit(bits))
					goto end;
				if (planes[layer*3+chan] < 0)
					missing[layer*3+chan] = planes[layer*3+chan] = get_vli(bits);
				int plane = planes[layer*3] - (layers-layer);
				if (plane >= 0 && plane < planes[layer*3+chan]) {
					decode(bits, buf, num, plane);
					--missing[layer*3+chan];
				}
			}
		}
	}
end:
	for (int len = lmin/2, num = len*len*cols*rows*3, *buf = buffer+num, layer = 0;
	len <= length/2; len *= 2, num = len*len*cols*rows*3, ++layer)
		for (int chan = 0; chan < 3; ++chan, buf += num)
			if (planes[layer*3+chan] > 0)
				finalize(buf, num, planes[layer*3+chan]);
	close_reader(bits);
	struct image *image = new_image(argv[2], width, height);
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	for (int chan = 0; chan < 3; ++chan) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				quantization(input, buffer, planes+chan, missing+chan, length, lmin, quant[chan], col, row, cols, rows, chan, 3);
				transformation(output, input, length, lmin, wavelet);
				copy(image->buffer+chan, output, width, height, length, col, row, cols, rows, 3);
			}
		}
	}
	free(buffer);
	free(input);
	free(output);
	for (int i = 0; i < width * height; ++i)
		image->buffer[3*i] += 0.5f;
	rgb_image(image);
	if (!write_ppm(image))
		return 1;
	delete_image(image);
	return 0;
}

