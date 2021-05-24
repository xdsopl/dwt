/*
Variable length integer coding

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

struct vli_reader {
	struct bits_reader *bits;
};

struct vli_writer {
	struct bits_writer *bits;
};

struct vli_reader *vli_reader(struct bits_reader *bits)
{
	struct vli_reader *vli = malloc(sizeof(struct vli_reader));
	vli->bits = bits;
	return vli;
}

struct vli_writer *vli_writer(struct bits_writer *bits)
{
	struct vli_writer *vli = malloc(sizeof(struct vli_writer));
	vli->bits = bits;
	return vli;
}

void delete_vli_reader(struct vli_reader *vli)
{
	free(vli);
}

void delete_vli_writer(struct vli_writer *vli)
{
	free(vli);
}

int vli_put_bit(struct vli_writer *vli, int bit)
{
	return put_bit(vli->bits, bit);
}

int vli_get_bit(struct vli_reader *vli)
{
	return get_bit(vli->bits);
}

int vli_write_bits(struct vli_writer *vli, int b, int n)
{
	return write_bits(vli->bits, b, n);
}

int vli_read_bits(struct vli_reader *vli, int *b, int n)
{
	return read_bits(vli->bits, b, n);
}

int put_vli(struct vli_writer *vli, int val)
{
	int cnt = 0, top = 1;
	while (top <= val) {
		cnt += 1;
		top = 1 << cnt;
		int ret = put_bit(vli->bits, 1);
		if (ret)
			return ret;
	}
	int ret = put_bit(vli->bits, 0);
	if (ret)
		return ret;
	if (cnt > 0) {
		cnt -= 1;
		val -= top/2;
		int ret = write_bits(vli->bits, val, cnt);
		if (ret)
			return ret;
	}
	return 0;
}

int get_vli(struct vli_reader *vli)
{
	int val = 0, cnt = 0, top = 1, ret;
	while ((ret = get_bit(vli->bits)) == 1) {
		cnt += 1;
		top = 1 << cnt;
	}
	if (ret < 0)
		return ret;
	if (cnt > 0) {
		cnt -= 1;
		if ((ret = read_bits(vli->bits, &val, cnt)))
			return ret;
		val += top/2;
	}
	return val;
}

