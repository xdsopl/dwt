/*
Arithmetic coding of binary streams

Copyright 2025 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

static const int ac_factor = 64; // 2 .. 64
static const int ac_code_bits = 16;
static const int ac_max_value = (1 << ac_code_bits) - 1;
static const int ac_first_half = 1 << (ac_code_bits - 1);
static const int ac_first_quarter = 1 << (ac_code_bits - 2);
static const int ac_last_quarter = ac_first_quarter | ac_first_half;

struct ac_reader {
	struct bits_reader *bits;
	long past[3];
	int freq[3];
	int count;
	int value;
	int lower;
	int upper;
};

struct ac_writer {
	struct bits_writer *bits;
	long past[3];
	int freq[3];
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
	for (int i = 0; i < 3; ++i)
		ac->past[i] = 0x5555555555555555L;
	for (int i = 0; i < 3; ++i)
		ac->freq[i] = ac_factor / 2;
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
	for (int i = 0; i < 3; ++i)
		ac->past[i] = 0x5555555555555555L;
	for (int i = 0; i < 3; ++i)
		ac->freq[i] = ac_factor / 2;
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

int ac_encode(struct ac_writer *ac, int bit, int freq)
{
	if (ac->count < 0)
		return ac->count;
	int range = ac->upper - ac->lower + 1;
	int point = range * freq;
	int offset = point / ac_factor;
	if (bit)
		ac->lower += offset;
	else
		ac->upper = ac->lower + offset - 1;
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

int ac_decode(struct ac_reader *ac, int freq)
{
	int range = ac->upper - ac->lower + 1;
	int point = range * freq;
	int bit = point < (ac->value - ac->lower + 1) * ac_factor;
	int offset = point / ac_factor;
	if (bit)
		ac->lower += offset;
	else
		ac->upper = ac->lower + offset - 1;
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
	return bit;
}

int ac_put_bit(struct ac_writer *ac, int bit)
{
	return ac_encode(ac, bit, ac_factor / 2);
}

int ac_get_bit(struct ac_reader *ac)
{
	return ac_decode(ac, ac_factor / 2);
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

void ac_update_freq64(long *past, int *freq, int bit)
{
	if (!bit)
		*freq += 1;
	if (*past & (1L << (ac_factor - 1)))
		*freq -= 1;
	*past <<= 1;
	*past |= !bit;
}

int ac_clamp(int x, int a, int b)
{
	return x < a ? a : x > b ? b : x;
}

int put_ac(struct ac_writer *ac, int bit, int ctx)
{
	int ret = ac_encode(ac, bit, ac_clamp(ac->freq[ctx], 1, ac_factor - 1));
	if (ret)
		return ret;
	ac_update_freq64(ac->past + ctx, ac->freq + ctx, bit);
	return 0;
}

int get_ac(struct ac_reader *ac, int ctx)
{
	int bit = ac_decode(ac, ac_clamp(ac->freq[ctx], 1, ac_factor - 1));
	if (bit < 0)
		return bit;
	ac_update_freq64(ac->past + ctx, ac->freq + ctx, bit);
	return bit;
}

