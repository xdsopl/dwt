/*
Arithmetic coding of binary streams

Copyright 2025 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

#define AC_FACTOR 256
static const int ac_rle_cnt = AC_FACTOR / 2;
static const int ac_code_bits = 16;
static const int ac_max_value = (1 << ac_code_bits) - 1;
static const int ac_first_half = 1 << (ac_code_bits - 1);
static const int ac_first_quarter = 1 << (ac_code_bits - 2);
static const int ac_last_quarter = ac_first_quarter | ac_first_half;

struct ac_reader {
	struct bits_reader *bits;
	int past[3 * AC_FACTOR];
	int freq[3];
	int index[3];
	int count;
	int value;
	int lower;
	int upper;
};

struct ac_writer {
	struct bits_writer *bits;
	int past[3 * AC_FACTOR];
	int freq[3];
	int index[3];
	int count;
	int lower;
	int upper;
};

int ac_get_abit(struct ac_reader *ac)
{
	if (ac->count) {
		if (ac->count < 0)
			return ac->count;
		ac->count += 1;
		if (ac->count > ac_code_bits - 2)
			return ac->count = -1;
		return 0;
	}
	int bit = get_bit(ac->bits);
	if (bit < 0) {
		ac->count = 1;
		return 0;
	}
	return bit;
}

struct ac_reader *ac_reader(struct bits_reader *bits)
{
	struct ac_reader *ac = malloc(sizeof(struct ac_reader));
	ac->bits = bits;
	for (int i = 0; i < AC_FACTOR - 1; ++i)
		ac->past[i] = 1;
	ac->past[AC_FACTOR - 1] = 0;
	ac->freq[0] = AC_FACTOR - 1;
	for (int i = AC_FACTOR; i < 3 * AC_FACTOR; ++i)
		ac->past[i] = i & 1;
	for (int i = 1; i < 3; ++i)
		ac->freq[i] = AC_FACTOR / 2;
	for (int i = 0; i < 3; ++i)
		ac->index[i] = 0;
	ac->count = 0;
	ac->value = 0;
	ac->lower = 0;
	ac->upper = ac_max_value;
	for (int i = 0; i < ac_code_bits; ++i) {
		ac->value <<= 1;
		ac->value |= ac_get_abit(ac);
	}
	return ac;
}

struct ac_writer *ac_writer(struct bits_writer *bits)
{
	struct ac_writer *ac = malloc(sizeof(struct ac_writer));
	ac->bits = bits;
	for (int i = 0; i < AC_FACTOR - 1; ++i)
		ac->past[i] = 1;
	ac->past[AC_FACTOR - 1] = 0;
	ac->freq[0] = AC_FACTOR - 1;
	for (int i = AC_FACTOR; i < 3 * AC_FACTOR; ++i)
		ac->past[i] = i & 1;
	for (int i = 1; i < 3; ++i)
		ac->freq[i] = AC_FACTOR / 2;
	for (int i = 0; i < 3; ++i)
		ac->index[i] = 0;
	ac->count = 0;
	ac->lower = 0;
	ac->upper = ac_max_value;
	return ac;
}

void delete_ac_reader(struct ac_reader *ac)
{
	free(ac);
}

int ac_put_bits(struct ac_writer *ac, int bit)
{
	if (ac->count < 0)
		return ac->count;
	int ret;
	if ((ret = put_bit(ac->bits, bit)))
		return ac->count = ret;
	for (int i = ac->count; i; --i)
		if ((ret = put_bit(ac->bits, !bit)))
			return ac->count = ret;
	return ac->count = 0;
}

void delete_ac_writer(struct ac_writer *ac)
{
	if (ac->count >= 0) {
		ac->count += 1;
		ac_put_bits(ac, ac->lower >= ac_first_quarter);
	}
	free(ac);
}

int ac_encode(struct ac_writer *ac, int symbol, int freq, int binary)
{
	if (ac->count < 0)
		return ac->count;
	int range = ac->upper - ac->lower + 1;
	int point = range * freq;
	int offset = point / AC_FACTOR;
	if (binary) {
		if (symbol)
			ac->lower += offset;
		else
			ac->upper = ac->lower + offset - 1;
	} else {
		int point1 = range * (AC_FACTOR - 1);
		int offset1 = point1 / AC_FACTOR;
		if (symbol == 0) {
			ac->upper = ac->lower + offset - 1;
		} else if (symbol == 1) {
			ac->upper = ac->lower + offset1 - 1;
			ac->lower += offset;
		} else if (symbol == 2) {
			ac->lower += offset1;
		} else {
			return -1;
		}
	}
	while (1) {
		if (ac->upper < ac_first_half) {
			if (ac_put_bits(ac, 0))
				return ac->count;
		} else if (ac->lower >= ac_first_half) {
			if (ac_put_bits(ac, 1))
				return ac->count;
			ac->lower -= ac_first_half;
			ac->upper -= ac_first_half;
		} else if (ac->lower >= ac_first_quarter && ac->upper < ac_last_quarter) {
			ac->count += 1;
			ac->lower -= ac_first_quarter;
			ac->upper -= ac_first_quarter;
		} else {
			break;
		}
		ac->lower <<= 1;
		ac->upper <<= 1;
		ac->upper |= 1;
	}
	return 0;
}

int ac_decode(struct ac_reader *ac, int freq, int binary)
{
	int value = (ac->value - ac->lower + 1) * AC_FACTOR;
	int range = ac->upper - ac->lower + 1;
	int point = range * freq;
	int offset = point / AC_FACTOR;
	int symbol;
	if (binary) {
		symbol = value > point;
		if (symbol)
			ac->lower += offset;
		else
			ac->upper = ac->lower + offset - 1;
	} else {
		int point1 = range * (AC_FACTOR - 1);
		int offset1 = point1 / AC_FACTOR;
		if (value <= point) {
			symbol = 0;
			ac->upper = ac->lower + offset - 1;
		} else if (value <= point1) {
			symbol = 1;
			ac->upper = ac->lower + offset1 - 1;
			ac->lower += offset;
		} else {
			symbol = 2;
			ac->lower += offset1;
		}
	}
	while (1) {
		if (ac->upper < ac_first_half) {
			// nothing to see here
		} else if (ac->lower >= ac_first_half) {
			ac->value -= ac_first_half;
			ac->lower -= ac_first_half;
			ac->upper -= ac_first_half;
		} else if (ac->lower >= ac_first_quarter && ac->upper < ac_last_quarter) {
			ac->value -= ac_first_quarter;
			ac->lower -= ac_first_quarter;
			ac->upper -= ac_first_quarter;
		} else {
			break;
		}
		ac->lower <<= 1;
		ac->upper <<= 1;
		ac->upper |= 1;
		ac->value <<= 1;
		int ret = ac_get_abit(ac);
		if (ret < 0)
			return ret;
		ac->value |= ret;
	}
	return symbol;
}

int ac_put_bit(struct ac_writer *ac, int bit)
{
	return ac_encode(ac, bit, AC_FACTOR / 2, 1);
}

int ac_get_bit(struct ac_reader *ac)
{
	return ac_decode(ac, AC_FACTOR / 2, 1);
}

int ac_write_bits(struct ac_writer *ac, int bits, int num)
{
	for (int i = 0; i < num; ++i) {
		int ret = ac_put_bit(ac, (bits >> i) & 1);
		if (ret)
			return ret;
	}
	return 0;
}

int ac_read_bits(struct ac_reader *ac, int *bits, int num)
{
	int acc = 0;
	for (int i = 0; i < num; ++i) {
		int bit = ac_get_bit(ac);
		if (bit < 0)
			return bit;
		acc |= bit << i;
	}
	*bits = acc;
	return 0;
}

void ac_update_freq(int *index, int *past, int *freq, int bit)
{
	*freq += !bit - past[*index];
	past[*index] = !bit;
	*index += 1;
	if (*index >= AC_FACTOR)
		*index = 0;
}

int ac_clamp(int x, int a, int b)
{
	return x < a ? a : x > b ? b : x;
}

int ac_put_val(struct ac_writer *ac, int val)
{
	static int order;
	while (val >= 1 << order) {
		if (ac_put_bit(ac, 0))
			return -1;
		val -= 1 << order;
		order += 1;
	}
	if (ac_put_bit(ac, 1))
		return -1;
	if (ac_write_bits(ac, val, order))
		return -1;
	order -= 1;
	if (order < 0)
		order = 0;
	return 0;
}

int ac_get_val(struct ac_reader *ac)
{
	static int order;
	int val, sum = 0, ret;
	while ((ret = ac_get_bit(ac)) == 0) {
		sum += 1 << order;
		order += 1;
	}
	if (ret < 0)
		return -1;
	if (ac_read_bits(ac, &val, order))
		return -1;
	order -= 1;
	if (order < 0)
		order = 0;
	return val + sum;
}

int put_ac(struct ac_writer *ac, int bit, int ctx)
{
	static int cnt, prv = -1;
	if ((!prv && ctx) || (!ctx && bit)) {
		if (cnt < ac_rle_cnt) {
			while (cnt--) {
				int ret = ac_encode(ac, 0, ac_clamp(ac->freq[0], 1, AC_FACTOR - 2), 0);
				if (ret)
					return ret;
				ac_update_freq(ac->index, ac->past, ac->freq, 0);
			}
		} else {
			int ret = ac_encode(ac, 2, ac_clamp(ac->freq[0], 1, AC_FACTOR - 2), 0);
			if (ret)
				return ret;
			ret = ac_put_val(ac, cnt - ac_rle_cnt);
			if (ret)
				return ret;
		}
		cnt = 0;
	}
	prv = ctx;
	if (ctx) {
		int ret = ac_encode(ac, bit, ac_clamp(ac->freq[ctx], 1, AC_FACTOR - 1), 1);
		if (ret == 0)
			ac_update_freq(ac->index + ctx, ac->past + ctx * AC_FACTOR, ac->freq + ctx, bit);
		return ret;
	}
	if (bit) {
		int ret = ac_encode(ac, 1, ac_clamp(ac->freq[0], 1, AC_FACTOR - 2), 0);
		if (ret)
			return ret;
		ac_update_freq(ac->index, ac->past, ac->freq, 1);
	} else {
		++cnt;
	}
	return 0;
}

int get_ac(struct ac_reader *ac, int ctx)
{
	static int cnt;
	if (ctx) {
		int bit = ac_decode(ac, ac_clamp(ac->freq[ctx], 1, AC_FACTOR - 1), 1);
		if (bit >= 0)
			ac_update_freq(ac->index + ctx, ac->past + ctx * AC_FACTOR, ac->freq + ctx, bit);
		return bit;
	}
	if (!cnt) {
		int bit = ac_decode(ac, ac_clamp(ac->freq[0], 1, AC_FACTOR - 2), 0);
		if (bit < 2) {
			if (bit >= 0)
				ac_update_freq(ac->index, ac->past, ac->freq, bit);
			return bit;
		}
		cnt = ac_get_val(ac);
		if (cnt < 0)
			return cnt;
		cnt += ac_rle_cnt;
	}
	--cnt;
	return 0;
}

