/*
Read and write bits

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bytes.h"

struct bits_reader {
	struct bytes_reader *bytes;
	int acc;
	int cnt;
};

struct bits_writer {
	struct bytes_writer *bytes;
	int acc;
	int cnt;
};

struct bits_reader *bits_reader(struct bytes_reader *bytes)
{
	struct bits_reader *bits = malloc(sizeof(struct bits_reader));
	bits->bytes = bytes;
	bits->acc = 0;
	bits->cnt = 0;
	return bits;
}

struct bits_writer *bits_writer(struct bytes_writer *bytes)
{
	struct bits_writer *bits = malloc(sizeof(struct bits_writer));
	bits->bytes = bytes;
	bits->acc = 0;
	bits->cnt = 0;
	return bits;
}

int bits_count(struct bits_writer *bits)
{
	return bits->cnt + 8 * bytes_count(bits->bytes);
}

void close_bits_reader(struct bits_reader *bits)
{
	free(bits);
}

void close_bits_writer(struct bits_writer *bits)
{
	if (bits->cnt)
		put_byte(bits->bytes, bits->acc);
	free(bits);
}

int put_bit(struct bits_writer *bits, int b)
{
	bits->acc |= !!b << bits->cnt++;
	if (bits->cnt >= 8) {
		bits->cnt -= 8;
		int b = bits->acc;
		bits->acc >>= 8;
		return put_byte(bits->bytes, b);
	}
	return 0;
}

int write_bits(struct bits_writer *bits, int b, int n)
{
	for (int i = 0; i < n; ++i) {
		int ret = put_bit(bits, (b >> i) & 1);
		if (ret)
			return ret;
	}
	return 0;
}

int get_bit(struct bits_reader *bits)
{
	if (!bits->cnt) {
		int b = get_byte(bits->bytes);
		if (b < 0)
			return b;
		bits->acc = b;
		bits->cnt = 8;
	}
	int b = bits->acc & 1;
	bits->acc >>= 1;
	bits->cnt -= 1;
	return b;
}

int read_bits(struct bits_reader *bits, int *b, int n)
{
	int a = 0;
	for (int i = 0; i < n; ++i) {
		int b = get_bit(bits);
		if (b < 0)
			return b;
		a |= b << i;
	}
	*b = a;
	return 0;
}

