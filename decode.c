/*
Decoder for lossy and lossless image compression based on the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#include "hilbert.h"
#include "haar.h"
#include "cdf97.h"
#include "rint_haar.h"
#include "utils.h"
#include "dwt.h"
#include "ppm.h"
#include "rle.h"
#include "vli.h"
#include "bits.h"

void transformation(float *output, float *input, int lmin, int width, int height, int wavelet)
{
	void (*funcs[3])(float *, float *, int, int, int) = { ihaar, icdf97, rint_ihaar };
	idwt2d(funcs[wavelet], output, input, lmin, width, height, 1, 1, width);
}

void quantization(float *output, int *input, int *missing, int *widths, int *heights, int *lengths, int levels, int mode, int quant)
{
	int width = widths[levels];
	for (int y = 0; y < heights[0]; ++y) {
		for (int x = 0; x < widths[0]; ++x) {
			float v = *input++;
			v /= 1 << quant;
			output[width*y+x] = v;
		}
	}
	for (int l = 0; l < levels; ++l) {
		for (int i = 0; i < lengths[l+1] * lengths[l+1]; ++i) {
			struct position pos = hilbert(lengths[l+1], i);
			if ((pos.x >= widths[l] || pos.y >= heights[l]) &&
			pos.x < widths[l+1] && pos.y < heights[l+1]) {
				float v = *input++;
				float bias = 0.375f;
				bias *= 1 << missing[l];
				if (!mode || missing[l]) {
					if (v < 0.f)
						v -= bias;
					else if (v > 0.f)
						v += bias;
				}
				v /= 1 << quant;
				output[width*pos.y+pos.x] = v;
			}
		}
	}
}

void copy(float *output, float *input, int width, int height, int stride)
{
	for (int i = 0; i < width * height; ++i)
		output[i*stride] = input[i];
}

int decode(struct rle_reader *rle, int *val, int num, int plane)
{
	int int_bits = sizeof(int) * 8;
	int sgn_pos = int_bits - 1;
	int sig_pos = int_bits - 2;
	int ref_pos = int_bits - 3;
	int sig_mask = 1 << sig_pos;
	int ref_mask = 1 << ref_pos;
	for (int i = 0; i < num; ++i) {
		if (!(val[i] & ref_mask)) {
			int bit = get_rle(rle);
			if (bit < 0)
				return bit;
			val[i] |= bit << plane;
			if (bit) {
				int sgn = rle_get_bit(rle);
				if (sgn < 0)
					return sgn;
				val[i] |= (sgn << sgn_pos) | sig_mask;
			}
		}
	}
	for (int i = 0; i < num; ++i) {
		if (val[i] & ref_mask) {
			int bit = rle_get_bit(rle);
			if (bit < 0)
				return bit;
			val[i] |= bit << plane;
		} else if (val[i] & sig_mask) {
			val[i] ^= sig_mask | ref_mask;
		}
	}
	return 0;
}

void process(int *val, int num)
{
	int int_bits = sizeof(int) * 8;
	int sgn_pos = int_bits - 1;
	int sig_pos = int_bits - 2;
	int ref_pos = int_bits - 3;
	int sgn_mask = 1 << sgn_pos;
	int sig_mask = 1 << sig_pos;
	int ref_mask = 1 << ref_pos;
	for (int i = 0; i < num; ++i) {
		val[i] &= ~(sig_mask|ref_mask);
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
	int mode = get_vli(vli);
	int wavelet = get_vli(vli);
	int width = get_vli(vli);
	int height = get_vli(vli);
	int lmin = get_vli(vli);
	if ((mode|wavelet|width|height|lmin) < 0)
		return 1;
	int quant[3];
	for (int chan = 0; chan < 3; ++chan)
		if ((quant[chan] = get_vli(vli)) < 0)
			return 1;
	int lengths[16], widths[16], heights[16];
	int levels = compute_lengths(lengths, widths, heights, width, height, lmin);
	int pixels_root = widths[0] * heights[0];
	int pixels = width * height;
	int *buffer = malloc(sizeof(int) * 3 * pixels);
	for (int i = 0; i < 3 * pixels; ++i)
		buffer[i] = 0;
	for (int chan = 0; chan < 3; ++chan)
		if (decode_root(vli, buffer+chan*pixels, pixels_root))
			return 1;
	int planes[3];
	for (int chan = 0; chan < 3; ++chan)
		if ((planes[chan] = get_vli(vli)) < 0)
			return 1;
	int planes_max = 0;
	for (int chan = 0; chan < 3; ++chan)
		if (planes_max < planes[chan])
			planes_max = planes[chan];
	int maximum = levels > planes_max ? levels : planes_max;
	int layers_max = 2 * maximum - 1;
	int missing[3*levels];
	for (int chan = 0; chan < 3; ++chan)
		for (int i = 0; i < levels; ++i)
			missing[chan*levels+i] = planes[chan];
	struct rle_reader *rle = rle_reader(vli);
	if (planes_max == planes[0]) {
		int num = widths[1] * heights[1] - pixels_root;
		if (decode(rle, buffer+pixels_root, num, planes[0]-1))
			goto end;
		--missing[0];
	}
	for (int layers = 0; layers < layers_max; ++layers) {
		for (int l = 0, *buf = buffer+pixels_root,
		num = widths[l+1] * heights[l+1] - widths[l] * heights[l];
		l < levels && l <= layers+1; buf += num, ++l,
		num = widths[l+1] * heights[l+1] - widths[l] * heights[l]) {
			for (int chan = 0; chan < 1; ++chan) {
				int plane = planes_max-1 - (layers+1-l);
				if (plane < 0 || plane >= planes[chan])
					continue;
				if (decode(rle, buf+chan*pixels, num, plane))
					goto end;
				--missing[chan*levels+l];
			}
		}
		for (int l = 0, *buf = buffer+pixels_root,
		num = widths[l+1] * heights[l+1] - widths[l] * heights[l];
		l < levels && l <= layers; buf += num, ++l,
		num = widths[l+1] * heights[l+1] - widths[l] * heights[l]) {
			for (int chan = 1; chan < 3; ++chan) {
				int plane = planes_max-1 - (layers-l);
				if (plane < 0 || plane >= planes[chan])
					continue;
				if (decode(rle, buf+chan*pixels, num, plane))
					goto end;
				--missing[chan*levels+l];
			}
		}
	}
end:
	delete_rle_reader(rle);
	delete_vli_reader(vli);
	close_reader(bits);
	for (int chan = 0; chan < 3; ++chan)
		process(buffer+chan*pixels+pixels_root, pixels-pixels_root);
	struct image *image = new_image(argv[2], width, height);
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	for (int chan = 0; chan < 3; ++chan) {
		quantization(input, buffer+chan*pixels, missing+chan*levels, widths, heights, lengths, levels, mode, quant[chan]);
		transformation(output, input, lmin, width, height, wavelet);
		copy(image->buffer+chan, output, width, height, 3);
	}
	free(buffer);
	free(input);
	free(output);
	if (mode) {
		for (int i = 0; i < width * height; ++i)
			image->buffer[3*i] += 128.f;
		srgb_from_rct(image);
	} else {
		for (int i = 0; i < width * height; ++i)
			image->buffer[3*i] += 0.5f;
		srgb_from_ycbcr(image);
	}
	if (!write_ppm(image))
		return 1;
	delete_image(image);
	return 0;
}

