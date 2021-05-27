/*
Decoder for lossy image compression based on the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#include "haar.h"
#include "cdf97.h"
#include "dwt.h"
#include "ppm.h"
#include "rle.h"
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

void quantization(float *output, int *input, int *missing, int length, int lmin, int quant, int col, int row, int cols, int rows)
{
	for (int y = 0, *in = input+(lmin/2)*(lmin/2)*(cols*row+col); y < lmin/2; ++y) {
		for (int x = 0; x < lmin/2; ++x) {
			float v = *in++;
			v /= 1 << quant;
			output[length*y+x] = v;
		}
	}
	input += (lmin/2) * (lmin/2) * cols * rows;
	for (int len = lmin/2; len <= length/2; input += 3*len*len*cols*rows, len *= 2, ++missing) {
		for (int yoff = 0, *in = input+3*len*len*(cols*row+col); yoff < len*2; yoff += len) {
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

int decode(struct rle_reader *rle, int *val, int num, int plane)
{
	int int_bits = sizeof(int) * 8;
	int sgn_pos = int_bits - 1;
	int sig_pos = int_bits - 2;
	int sig_mask = 1 << sig_pos;
	for (int i = 0; i < num; ++i) {
		int bit = get_rle(rle);
		if (bit < 0)
			return bit;
		val[i] |= bit << plane;
		if (bit && !(val[i] & sig_mask)) {
			int sgn = rle_get_bit(rle);
			if (sgn < 0)
				return sgn;
			val[i] |= (sgn << sgn_pos) | sig_mask;
		}
	}
	return 0;
}

void process(int *val, int num)
{
	int int_bits = sizeof(int) * 8;
	int sgn_pos = int_bits - 1;
	int sig_pos = int_bits - 2;
	int sgn_mask = 1 << sgn_pos;
	int sig_mask = 1 << sig_pos;
	for (int i = 0; i < num; ++i) {
		val[i] &= ~sig_mask;
		if (val[i] & sgn_mask)
			val[i] = -(val[i]^sgn_mask);
	}
}

int decode_root(struct vli_reader *vli, int *val, int num)
{
	int cnt = get_vli(vli);
	if (cnt < 0)
		return cnt;
	for (int i = 0; cnt && i < num; ++i) {
		int ret = vli_read_bits(vli, val+i, cnt);
		if (ret)
			return ret;
		if (val[i] && (ret = vli_get_bit(vli)))
			val[i] = -val[i];
		if (ret < 0)
			return ret;
	}
	return 0;
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
	int coding = get_bit(bits);
	if (coding != 0)
		return 1;
	struct vli_reader *vli = vli_reader(bits);
	int wavelet = get_vli(vli);
	int width = get_vli(vli);
	int height = get_vli(vli);
	int depth = get_vli(vli);
	int dmin = get_vli(vli);
	int cols = get_vli(vli);
	int rows = get_vli(vli);
	if ((wavelet|width|height|depth|dmin|cols|rows) < 0)
		return 1;
	int quant[3];
	for (int chan = 0; chan < 3; ++chan)
		if ((quant[chan] = get_vli(vli)) < 0)
			return 1;
	int length = 1 << depth;
	int lmin = 1 << dmin;
	int pixels = length * length;
	int *buffer = malloc(sizeof(int) * 3 * pixels * rows * cols);
	for (int i = 0; i < 3 * pixels * rows * cols; ++i)
		buffer[i] = 0;
	int pixels_root = (lmin/2) * (lmin/2) * cols * rows;
	for (int chan = 0; chan < 3; ++chan)
		if (decode_root(vli, buffer+chan*pixels*rows*cols, pixels_root))
			return 1;
	int planes[3];
	for (int chan = 0; chan < 3; ++chan)
		if ((planes[chan] = get_vli(vli)) < 0)
			return 1;
	int planes_max = 0;
	for (int chan = 0; chan < 3; ++chan)
		if (planes_max < planes[chan])
			planes_max = planes[chan];
	int maximum = depth > planes_max ? depth : planes_max;
	int layers_max = 2 * maximum - 1;
	int missing[3*depth];
	for (int chan = 0; chan < 3; ++chan)
		for (int i = 0; i < depth; ++i)
			missing[chan*depth+i] = planes[chan];
	struct rle_reader *rle = rle_reader(vli);
	for (int layers = 0; layers < layers_max; ++layers) {
		for (int len = lmin/2, num = len*len*cols*rows, *buf = buffer+num, layer = 0;
		len <= length/2 && layer <= layers; len *= 2, buf += 3*num, num = len*len*cols*rows, ++layer) {
			for (int loops = 4, loop = 0; loop < loops; ++loop) {
				for (int chan = 0; chan < 1; ++chan) {
					int plane = planes_max-1 - ((layers-layer)*loops+loop);
					if (plane < 0 || plane >= planes[chan])
						continue;
					if (decode(rle, buf+chan*pixels*rows*cols, 3*num, plane))
						goto end;
					--missing[chan*depth+layer];
				}
			}
		}
		for (int len = lmin/2, num = len*len*cols*rows, *buf = buffer+num, layer = 0;
		len <= length/2 && layer <= layers; len *= 2, buf += 3*num, num = len*len*cols*rows, ++layer) {
			for (int loops = 4, loop = 0; loop < loops; ++loop) {
				for (int chan = 1; chan < 3; ++chan) {
					int plane = planes_max-1 - ((layers-layer)*loops+loop);
					if (plane < 0 || plane >= planes[chan])
						continue;
					if (decode(rle, buf+chan*pixels*rows*cols, 3*num, plane))
						goto end;
					--missing[chan*depth+layer];
				}
			}
		}
	}
end:
	delete_rle_reader(rle);
	delete_vli_reader(vli);
	close_reader(bits);
	for (int chan = 0; chan < 3; ++chan)
		process(buffer+chan*pixels*rows*cols+pixels_root, pixels*rows*cols-pixels_root);
	struct image *image = new_image(argv[2], width, height);
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	for (int chan = 0; chan < 3; ++chan) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				quantization(input, buffer+chan*pixels*rows*cols, missing+chan*depth, length, lmin, quant[chan], col, row, cols, rows);
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

