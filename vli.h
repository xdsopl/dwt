/*
Variable length integer Rice coding

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

struct vli_reader {
	struct bits_reader *bits;
	int k;
};

struct vli_writer {
	struct bits_writer *bits;
	int k;
};

struct vli_reader *vli_reader(struct bits_reader *bits)
{
	struct vli_reader *vli = malloc(sizeof(struct vli_reader));
	vli->bits = bits;
	vli->k = 3;
	return vli;
}

struct vli_writer *vli_writer(struct bits_writer *bits)
{
	struct vli_writer *vli = malloc(sizeof(struct vli_writer));
	vli->bits = bits;
	vli->k = 3;
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

int put_vli(struct vli_writer *vli, int x) {
	int k = vli->k, ret;
	for (int q = x >> k; q; --q)
		if ((ret = put_bit(vli->bits, 0)))
			return ret;
	if ((ret = put_bit(vli->bits, 1)))
		return ret;
	if ((ret = write_bits(vli->bits, x & ((1 << k) - 1), k)))
		return ret;
	return 0;
}

int get_vli(struct vli_reader *vli) {
	int k = vli->k, q = 0, r, ret;
	while ((ret = get_bit(vli->bits)) == 0)
		++q;
	if (ret < 0)
		return ret;
	if ((ret = read_bits(vli->bits, &r, k)))
		return ret;
	return (q << k) | r;
}

