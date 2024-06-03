/*
Decoder for lossless image compression based on the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#include "hilbert.h"
#include "cdf53.h"
#include "utils.h"
#include "pnm.h"
#include "rle.h"
#include "vli.h"
#include "bits.h"

void transformation(int *out, int *in, int N0, int W, int H, int SO, int SI, int SW, int CH)
{
	int W2 = (W+1)/2, H2 = (H+1)/2;
	if (W2 >= N0 && H2 >= N0)
		transformation(out, in, N0, W2, H2, SO, SI, SW, CH);
	icdf53(out, in, H, SO*SW, SI*SW, W*CH);
	for (int j = 0; j < H; ++j)
		for (int i = 0; i < W*CH; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	for (int j = 0; j < H; ++j) {
		icdf53(out+SO*SW*j, in+SI*SW*j, W, SO*CH, SI*CH, CH);
		for (int i = 0; i < W*CH; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	}
}

void reconstruction(int *output, int *input, int *missing, int *widths, int *heights, int *lengths, int levels, int channels)
{
	int width = widths[levels];
	int height = heights[levels];
	int pixels = width * height;
	for (int y = 0; y < heights[0]; ++y) {
		for (int x = 0; x < widths[0]; ++x) {
			for (int chan = 0; chan < channels; ++chan) {
				int v = input[chan*pixels];
				output[channels*(width*y+x)+chan] = v;
			}
			++input;
		}
	}
	for (int l = 0; l < levels; ++l) {
		for (int i = 0; i < lengths[l+1] * lengths[l+1]; ++i) {
			struct position pos = hilbert(lengths[l+1], i);
			if ((pos.x >= widths[l] || pos.y >= heights[l]) && pos.x < widths[l+1] && pos.y < heights[l+1]) {
				for (int chan = 0; chan < channels; ++chan) {
					int v = input[chan*pixels];
					int m = missing[chan*levels+l];
					if (m) {
						int bias = 1 << (m - 1);
						if (v < 0)
							v -= bias;
						else if (v > 0)
							v += bias;
					}
					output[channels*(width*pos.y+pos.x)+chan] = v;
				}
				++input;
			}
		}
	}
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
		fprintf(stderr, "usage: %s input.dwt output.pnm\n", argv[0]);
		return 1;
	}
	struct bits_reader *bits = bits_reader(argv[1]);
	if (!bits)
		return 1;
	int letter;
	if (read_bits(bits, &letter, 8) || letter != 'W')
		return 1;
	int number;
	if (read_bits(bits, &number, 8) || (number != '5' && number != '6'))
		return 1;
	int color = number == '6';
	int width, height;
	if (read_bits(bits, &width, 16) || read_bits(bits, &height, 16))
		return 1;
	++width;
	++height;
	int min_len = 8;
	if (width < min_len || height < min_len)
		return 1;
	struct vli_reader *vli = vli_reader(bits);
	int lengths[16], widths[16], heights[16];
	int levels = compute_lengths(lengths, widths, heights, width, height, min_len);
	int pixels_root = widths[0] * heights[0];
	int pixels = width * height;
	int channels = color ? 3 : 1;
	int *buffer = malloc(sizeof(int) * channels * pixels);
	for (int i = 0; i < channels * pixels; ++i)
		buffer[i] = 0;
	for (int chan = 0; chan < channels; ++chan)
		if (decode_root(vli, buffer+chan*pixels, pixels_root))
			return 1;
	int planes[channels];
	for (int chan = 0; chan < channels; ++chan)
		if ((planes[chan] = get_vli(vli)) < 0)
			return 1;
	int planes_max = 0;
	for (int chan = 0; chan < channels; ++chan)
		if (planes_max < planes[chan])
			planes_max = planes[chan];
	int maximum = levels > planes_max ? levels : planes_max;
	int layers_max = 2 * maximum - 1;
	int missing[channels*levels];
	for (int chan = 0; chan < channels; ++chan)
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
			for (int chan = 1; chan < channels; ++chan) {
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
	for (int chan = 0; chan < channels; ++chan)
		process(buffer+chan*pixels+pixels_root, pixels-pixels_root);
	struct image *image = new_image(argv[2], width, height, channels);
	int *temp = malloc(sizeof(int) * channels * pixels);
	reconstruction(temp, buffer, missing, widths, heights, lengths, levels, channels);
	transformation(image->buffer, temp, min_len, width, height, 1, 1, width * channels, channels);
	free(buffer);
	free(temp);
	if (color)
		rgb_from_ycocg(image);
	if (!write_pnm(image))
		return 1;
	delete_image(image);
	return 0;
}

