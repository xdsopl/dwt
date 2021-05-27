/*
Encoder for lossy image compression based on the discrete wavelet transformation

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
		dwt2d(cdf97, output, input, lmin, length, 1, 1);
	else
		haar2d(output, input, lmin, length, 1, 1);
}

void quantization(int *output, float *input, int length, int lmin, int quant, int col, int row, int cols, int rows)
{
	for (int y = 0, *out = output+(lmin/2)*(lmin/2)*(cols*row+col); y < lmin/2; ++y) {
		for (int x = 0; x < lmin/2; ++x) {
			float v = input[length*y+x];
			v *= 1 << quant;
			*out++ = nearbyintf(v);
		}
	}
	output += (lmin/2) * (lmin/2) * cols * rows;
	for (int len = lmin/2; len <= length/2; output += 3*len*len*cols*rows, len *= 2) {
		for (int yoff = 0, *out = output+3*len*len*(cols*row+col); yoff < len*2; yoff += len) {
			for (int xoff = !yoff * len; xoff < len*2; xoff += len) {
				for (int i = 0; i < len*len; ++i) {
					struct position pos = hilbert(len, i);
					float v = input[length*(yoff+pos.y)+xoff+pos.x];
					v *= 1 << quant;
					*out++ = truncf(v);
				}
			}
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

int encode(struct rle_writer *rle, int *val, int num, int plane)
{
	int bit_mask = 1 << plane;
	int int_bits = sizeof(int) * 8;
	int sgn_pos = int_bits - 1;
	int sig_pos = int_bits - 2;
	int ref_pos = int_bits - 3;
	int sgn_mask = 1 << sgn_pos;
	int sig_mask = 1 << sig_pos;
	int ref_mask = 1 << ref_pos;
	for (int i = 0; i < num; ++i) {
		if (!(val[i] & ref_mask)) {
			int bit = val[i] & bit_mask;
			int ret = put_rle(rle, bit);
			if (ret)
				return ret;
			if (bit) {
				int ret = rle_put_bit(rle, val[i] & sgn_mask);
				if (ret)
					return ret;
				val[i] |= sig_mask;
			}
		}
	}
	int ret = put_rle(rle, 1);
	if (ret)
		return ret;
	for (int i = 0; i < num; ++i) {
		if (val[i] & ref_mask) {
			int bit = val[i] & bit_mask;
			int ret = rle_put_bit(rle, bit);
			if (ret)
				return ret;
		} else if (val[i] & sig_mask) {
			val[i] ^= sig_mask | ref_mask;
		}
	}
	return 0;
}

int ilog2(int x)
{
	int l = -1;
	for (; x > 0; x /= 2)
		++l;
	return l;
}

void encode_root(struct vli_writer *vli, int *val, int num)
{
	int max = 0;
	for (int i = 0; i < num; ++i)
		if (max < abs(val[i]))
			max = abs(val[i]);
	int cnt = 1 + ilog2(max);
	put_vli(vli, cnt);
	for (int i = 0; cnt && i < num; ++i) {
		vli_write_bits(vli, abs(val[i]), cnt);
		if (val[i])
			vli_put_bit(vli, val[i] < 0);
	}
}

int process(int *val, int num)
{
	int max = 0;
	int int_bits = sizeof(int) * 8;
	int sgn_pos = int_bits - 1;
	int sig_pos = int_bits - 2;
	int ref_pos = int_bits - 3;
	int sgn_mask = 1 << sgn_pos;
	int sig_mask = 1 << sig_pos;
	int ref_mask = 1 << ref_pos;
	int mix_mask = sgn_mask | sig_mask | ref_mask;
	for (int i = 0; i < num; ++i) {
		int sgn = val[i] < 0;
		int mag = abs(val[i]);
		if (max < mag)
			max = mag;
		val[i] = (sgn << sgn_pos) | (mag & ~mix_mask);
	}
	return 1 + ilog2(max);
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
	int dmin = 2;
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
	int quant[3] = { 7, 5, 5 };
	if (argc >= 6)
		for (int chan = 0; chan < 3; ++chan)
			quant[chan] = atoi(argv[3+chan]);
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
	float *output = malloc(sizeof(float) * pixels);
	int *buffer = malloc(sizeof(int) * 3 * pixels * rows * cols);
	for (int chan = 0; chan < 3; ++chan) {
		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				copy(input, image->buffer+chan, width, height, length, col, row, cols, rows, 3);
				transformation(output, input, length, lmin, wavelet);
				quantization(buffer+chan*pixels*rows*cols, output, length, lmin, quant[chan], col, row, cols, rows);
			}
		}
	}
	delete_image(image);
	free(input);
	free(output);
	int pixels_root = (lmin/2) * (lmin/2) * cols * rows;
	int planes[3];
	for (int chan = 0; chan < 3; ++chan)
		planes[chan] = process(buffer+chan*pixels*rows*cols+pixels_root, pixels*rows*cols-pixels_root);
	struct bits_writer *bits = bits_writer(argv[2], capacity);
	if (!bits)
		return 1;
	put_bit(bits, 0);
	struct vli_writer *vli = vli_writer(bits);
	put_vli(vli, wavelet);
	put_vli(vli, width);
	put_vli(vli, height);
	put_vli(vli, depth);
	put_vli(vli, dmin);
	put_vli(vli, cols);
	put_vli(vli, rows);
	for (int chan = 0; chan < 3; ++chan)
		put_vli(vli, quant[chan]);
	int meta_data = bits_count(bits);
	fprintf(stderr, "%d bits for meta data\n", meta_data);
	for (int chan = 0; chan < 3; ++chan)
		encode_root(vli, buffer+chan*pixels*rows*cols, pixels_root);
	int root_image = bits_count(bits);
	fprintf(stderr, "%d bits for root image\n", root_image - meta_data);
	for (int chan = 0; chan < 3; ++chan)
		put_vli(vli, planes[chan]);
	int planes_max = 0;
	for (int chan = 0; chan < 3; ++chan)
		if (planes_max < planes[chan])
			planes_max = planes[chan];
	int maximum = depth > planes_max ? depth : planes_max;
	int layers_max = 2 * maximum - 1;
	struct rle_writer *rle = rle_writer(vli);
	for (int layers = 0; layers < layers_max; ++layers) {
		for (int len = lmin/2, num = len*len*cols*rows, *buf = buffer+num, layer = 0;
		len <= length/2 && layer <= layers; len *= 2, buf += 3*num, num = len*len*cols*rows, ++layer) {
			for (int loops = 4, loop = 0; loop < loops; ++loop) {
				for (int chan = 0; chan < 1; ++chan) {
					int plane = planes_max-1 - ((layers-layer)*loops+loop);
					if (plane < 0 || plane >= planes[chan])
						continue;
					if (encode(rle, buf+chan*pixels*rows*cols, 3*num, plane))
						goto end;
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
					if (encode(rle, buf+chan*pixels*rows*cols, 3*num, plane))
						goto end;
				}
			}
		}
	}
	rle_flush(rle);
end:
	delete_rle_writer(rle);
	delete_vli_writer(vli);
	free(buffer);
	int cnt = bits_count(bits);
	int bytes = (cnt + 7) / 8;
	int kib = (bytes + 512) / 1024;
	fprintf(stderr, "%d bits (%d KiB) encoded\n", cnt, kib);
	close_writer(bits);
	return 0;
}

