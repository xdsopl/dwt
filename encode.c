/*
Encoder for lossy and lossless image compression based on the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#include "hilbert.h"
#include "cdf53.h"
#include "haar.h"
#include "utils.h"
#include "dwt.h"
#include "ppm.h"
#include "rle.h"
#include "vli.h"
#include "bits.h"

void transformation(float *output, float *input, int lmin, int width, int height, int wavelet, int channels)
{
	void (*funcs[2])(float *, float *, int, int, int, int) = { cdf53, haar };
	dwt2d(funcs[wavelet], output, input, lmin, width, height, 1, 1, width * channels, channels);
}

void quantization(int *output, float *input, int *widths, int *heights, int *lengths, int levels, int channels)
{
	int width = widths[levels];
	int height = heights[levels];
	int pixels = width * height;
	for (int y = 0; y < heights[0]; ++y) {
		for (int x = 0; x < widths[0]; ++x) {
			for (int chan = 0; chan < channels; ++chan) {
				float v = input[channels*(width*y+x)+chan];
				output[chan*pixels] = nearbyintf(v);
			}
			++output;
		}
	}
	for (int l = 0; l < levels; ++l) {
		for (int i = 0; i < lengths[l+1] * lengths[l+1]; ++i) {
			struct position pos = hilbert(lengths[l+1], i);
			if ((pos.x >= widths[l] || pos.y >= heights[l]) && pos.x < widths[l+1] && pos.y < heights[l+1]) {
				for (int chan = 0; chan < channels; ++chan) {
					float v = input[channels*(width*pos.y+pos.x)+chan];
					output[chan*pixels] = truncf(v);
				}
				++output;
			}
		}
	}
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
	if (argc != 3 && argc != 4 && argc != 5) {
		fprintf(stderr, "usage: %s input.ppm output.dwt [CAPACITY] [WAVELET]\n", argv[0]);
		return 1;
	}
	struct image *image = read_ppm(argv[1]);
	if (!image)
		return 1;
	int width = image->width;
	int height = image->height;
	int pixels = width * height;
	int lmin = 4;
	int lengths[16], widths[16], heights[16];
	int levels = compute_lengths(lengths, widths, heights, width, height, lmin);
	int pixels_root = widths[0] * heights[0];
	int capacity = 0;
	if (argc >= 4)
		capacity = atoi(argv[3]);
	int wavelet = 0;
	if (argc >= 5)
		wavelet = atoi(argv[4]);
	ycocg_from_rgb(image);
	int channels = 3;
	for (int i = 0; i < width * height; ++i)
		image->buffer[channels*i] -= 128.f;
	float *temp = malloc(sizeof(float) * channels * pixels);
	int *buffer = malloc(sizeof(int) * channels * pixels);
	transformation(temp, image->buffer, lmin, width, height, wavelet, channels);
	quantization(buffer, temp, widths, heights, lengths, levels, channels);
	delete_image(image);
	free(temp);
	int planes[channels];
	for (int chan = 0; chan < channels; ++chan)
		planes[chan] = process(buffer+chan*pixels+pixels_root, pixels-pixels_root);
	struct bits_writer *bits = bits_writer(argv[2], capacity);
	if (!bits)
		return 1;
	put_bit(bits, 0);
	struct vli_writer *vli = vli_writer(bits);
	put_vli(vli, wavelet);
	put_vli(vli, width);
	put_vli(vli, height);
	put_vli(vli, lmin);
	int meta_data = bits_count(bits);
	fprintf(stderr, "%d bits for meta data\n", meta_data);
	for (int chan = 0; chan < channels; ++chan)
		encode_root(vli, buffer+chan*pixels, pixels_root);
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
		int num = widths[1] * heights[1] - pixels_root;
		if (encode(rle, buffer+pixels_root, num, planes[0]-1))
			goto end;
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
				if (encode(rle, buf+chan*pixels, num, plane))
					goto end;
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
				if (encode(rle, buf+chan*pixels, num, plane))
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
	int bytes = (cnt + 7) / 8;
	int kib = (bytes + 512) / 1024;
	fprintf(stderr, "%d bits (%d KiB) encoded\n", cnt, kib);
	close_writer(bits);
	return 0;
}

