/*
Variable length integer Rice coding

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

struct vli_reader {
	struct bits_reader *bits;
	int *hist;
	int *perm;
	int order;
};

struct vli_writer {
	struct bits_writer *bits;
	int *hist;
	int *perm;
	int order;
};

struct vli_reader *vli_reader(struct bits_reader *bits)
{
	struct vli_reader *vli = malloc(sizeof(struct vli_reader));
	vli->hist = calloc(64, sizeof(int));
	vli->perm = calloc(64, sizeof(int));
	for (int i = 0; i < 64; ++i)
		vli->perm[i] = i;
	vli->bits = bits;
	vli->order = 0;
	return vli;
}

struct vli_writer *vli_writer(struct bits_writer *bits)
{
	struct vli_writer *vli = malloc(sizeof(struct vli_writer));
	vli->hist = calloc(64, sizeof(int));
	vli->perm = calloc(64, sizeof(int));
	for (int i = 0; i < 64; ++i)
		vli->perm[i] = i;
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

void vli_swp(int *a, int *b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void vli_sort(int *p, int *a, int n)
{
	p[0] = 0;
	for (int i = 1, j; i < n; ++i) {
		int t = a[i];
		for (j = i; j > 0 && t > a[j-1]; --j) {
			a[j] = a[j-1];
			p[j] = p[j-1];
		}
		a[j] = t;
		p[j] = i;
	}
}

int vli_map(int *hist, int *perm, int val)
{
	if (val >= 64)
		return val;
	int map = perm[val];
	++hist[map];
	vli_sort(perm, hist, 64);
	return map;
}

int put_vli(struct vli_writer *vli, int val)
{
	val = vli_map(vli->hist, vli->perm, val);
	int ret;
	while (val >= 1 << vli->order) {
		if ((ret = put_bit(vli->bits, 0)))
			return ret;
		val -= 1 << vli->order;
		vli->order += 1;
	}
	if ((ret = put_bit(vli->bits, 1)))
		return ret;
	if ((ret = write_bits(vli->bits, val, vli->order)))
		return ret;
	vli->order -= 2;
	if (vli->order < 0)
		vli->order = 0;
	return 0;
}

int get_vli(struct vli_reader *vli)
{
	int val, sum = 0, ret;
	while ((ret = get_bit(vli->bits)) == 0) {
		sum += 1 << vli->order;
		vli->order += 1;
	}
	if (ret < 0)
		return ret;
	if ((ret = read_bits(vli->bits, &val, vli->order)))
		return ret;
	vli->order -= 2;
	if (vli->order < 0)
		vli->order = 0;
	return vli_map(vli->hist, vli->perm, val + sum);
}

