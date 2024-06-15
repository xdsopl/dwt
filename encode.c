/*
Encoder for lossless image compression based on the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#include "hilbert.h"
#include "cdf53.h"
#include "utils.h"
#include "pnm.h"
#include "rle.h"
#include "vli.h"
#include "bits.h"
#include "bytes.h"

void transformation(int *out, int *in, int N0, int W, int H, int SO, int SI, int SW, int CH)
{
	for (int j = 0; j < H; ++j) {
		cdf53(out + SO * SW * j, in + SI * SW * j, W, SO * CH, SI * CH, CH);
		for (int i = 0; i < W * CH; ++i)
			in[(SW * j + i) * SI] = out[(SW * j + i) * SO];
	}
	cdf53(out, in, H, SO * SW, SI * SW, W * CH);
	int W2 = (W + 1) / 2, H2 = (H + 1) / 2;
	for (int j = 0; j < H2; ++j)
		for (int i = 0; i < W2 * CH; ++i)
			in[(SW * j + i) * SI] = out[(SW * j + i) * SO];
	if (W2 >= N0 && H2 >= N0)
		transformation(out, in, N0, W2, H2, SO, SI, SW, CH);
}

void linearization(int *output, int *input, int *widths, int *heights, int *lengths, int levels, int channels)
{
	int width = widths[levels];
	int height = heights[levels];
	int total = width * height;
	for (int y = 0; y < heights[0]; ++y) {
		for (int x = 0; x < widths[0]; ++x) {
			for (int chan = 0; chan < channels; ++chan) {
				int v = input[channels * (width * y + x) + chan];
				output[chan * total] = v;
			}
			++output;
		}
	}
	for (int l = 0; l < levels; ++l) {
		for (int i = 0; i < lengths[l + 1] * lengths[l + 1]; ++i) {
			struct position pos = hilbert(lengths[l + 1], i);
			if ((pos.x >= widths[l] || pos.y >= heights[l]) && pos.x < widths[l + 1] && pos.y < heights[l + 1]) {
				for (int chan = 0; chan < channels; ++chan) {
					int v = input[channels * (width * pos.y + pos.x) + chan];
					output[chan * total] = v;
				}
				++output;
			}
		}
	}
}

int encode_plane(struct rle_writer *rle, int *val, int num, int plane)
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
	if (argc != 3 && argc != 4) {
		fprintf(stderr, "usage: %s input.pnm output.dwt [CAPACITY]\n", argv[0]);
		return 1;
	}
	struct image *image = read_pnm(argv[1]);
	if (!image || image->width > 65536 || image->height > 65536)
		return 1;
	int width = image->width;
	int height = image->height;
	int min_len = 8;
	if (width < min_len || height < min_len)
		return 1;
	int total = width * height;
	int lengths[16], pixels[16], widths[16], heights[16];
	int levels = compute_lengths(lengths, pixels, widths, heights, width, height, min_len);
	int capacity = 0;
	if (argc >= 4)
		capacity = atoi(argv[3]);
	int channels = image->channels;
	int color = channels == 3;
	if (color)
		ycocg_from_rgb(image);
	int *temp = malloc(sizeof(int) * channels * total);
	int *buffer = malloc(sizeof(int) * channels * total);
	transformation(temp, image->buffer, min_len, width, height, 1, 1, width * channels, channels);
	linearization(buffer, temp, widths, heights, lengths, levels, channels);
	delete_image(image);
	free(temp);
	int planes[channels];
	for (int chan = 0; chan < channels; ++chan)
		planes[chan] = process(buffer + chan * total + pixels[0], total - pixels[0]);
	struct bytes_writer *bytes = bytes_writer(argv[2], capacity);
	if (!bytes)
		return 1;
	put_byte(bytes, 'W');
	put_byte(bytes, color ? '6' : '5');
	write_bytes(bytes, width - 1, 2);
	write_bytes(bytes, height - 1, 2);
	struct bits_writer *bits = bits_writer(bytes);
	struct vli_writer *vli = vli_writer(bits);
	int meta_data = bits_count(bits);
	fprintf(stderr, "%d bits for meta data\n", meta_data);
	for (int chan = 0; chan < channels; ++chan)
		encode_root(vli, buffer + chan * total, pixels[0]);
	int root_image = bits_count(bits);
	fprintf(stderr, "%d bits for root image\n", root_image - meta_data);
	for (int chan = 0; chan < channels; ++chan)
		put_vli(vli, planes[chan]);
	int planes_max = 0;
	for (int chan = 0; chan < channels; ++chan)
		if (planes_max < planes[chan])
			planes_max = planes[chan];
	int maximum = levels > planes_max ? levels : planes_max;
	int layers_max = 2 * maximum - 1;
	struct rle_writer *rle = rle_writer(vli);
	if (planes_max == planes[0]) {
		int num = pixels[1] - pixels[0];
		if (encode_plane(rle, buffer + pixels[0], num, planes[0] - 1))
			goto end;
	}
	for (int layers = 0; layers < layers_max; ++layers) {
		for (int l = 0, *buf = buffer + pixels[0],
			num = pixels[l + 1] - pixels[l];
			l < levels && l <= layers + 1; buf += num, ++l,
			num = pixels[l + 1] - pixels[l]) {
			for (int chan = 0; chan < 1; ++chan) {
				int plane = planes_max - 1 - (layers + 1 - l);
				if (plane < 0 || plane >= planes[chan])
					continue;
				if (encode_plane(rle, buf + chan * total, num, plane))
					goto end;
			}
		}
		for (int l = 0, *buf = buffer + pixels[0],
			num = pixels[l + 1] - pixels[l];
			l < levels && l <= layers; buf += num, ++l,
			num = pixels[l + 1] - pixels[l]) {
			for (int chan = 1; chan < channels; ++chan) {
				int plane = planes_max - 1 - (layers - l);
				if (plane < 0 || plane >= planes[chan])
					continue;
				if (encode_plane(rle, buf + chan * total, num, plane))
					goto end;
			}
		}
	}
	rle_flush(rle);
end:
	delete_rle_writer(rle);
	delete_vli_writer(vli);
	free(buffer);
	int cnt = bits_count(bits);
	close_bits_writer(bits);
	int kib = (bytes_count(bytes) + 512) / 1024;
	close_bytes_writer(bytes);
	fprintf(stderr, "%d bits (%d KiB) encoded\n", cnt, kib);
	return 0;
}

