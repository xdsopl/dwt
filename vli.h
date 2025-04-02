/*
Variable length integer Rice coding

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

struct vli_reader {
	struct bits_reader *bits;
	int order;
};

struct vli_writer {
	struct bits_writer *bits;
	int order;
};

struct vli_reader *vli_reader(struct bits_reader *bits)
{
	struct vli_reader *vli = malloc(sizeof(struct vli_reader));
	vli->bits = bits;
	vli->order = 0;
	return vli;
}

struct vli_writer *vli_writer(struct bits_writer *bits)
{
	struct vli_writer *vli = malloc(sizeof(struct vli_writer));
	vli->bits = bits;
	vli->order = 0;
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
	int ret, cnt = vli->order;
	while (val >= 1 << cnt) {
		if ((ret = put_bit(vli->bits, 0)))
			return ret;
		val -= 1 << cnt;
		cnt += 1;
	}
	if ((ret = put_bit(vli->bits, 1)))
		return ret;
	if ((ret = write_bits(vli->bits, val, cnt)))
		return ret;
	cnt -= 1;
	if (cnt < 0)
		cnt = 0;
	vli->order = (vli->order + cnt) / 2;
	return 0;
}

int get_vli(struct vli_reader *vli)
{
	int cnt = vli->order, sum = 0, ret, val;
	while ((ret = get_bit(vli->bits)) == 0) {
		sum += 1 << cnt;
		cnt += 1;
	}
	if (ret < 0)
		return ret;
	if ((ret = read_bits(vli->bits, &val, cnt)))
		return ret;
	cnt -= 1;
	if (cnt < 0)
		cnt = 0;
	vli->order = (vli->order + cnt) / 2;
	return val + sum;
}

