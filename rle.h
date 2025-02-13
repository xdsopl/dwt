/*
Run length encoding of mostly zero bits

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "vli.h"

struct rle_reader {
	struct vli_reader *vli;
	int cnt;
	int val;
};

struct rle_writer {
	struct vli_writer *vli;
	int cnt;
	int val;
};

struct rle_reader *rle_reader(struct vli_reader *vli)
{
	struct rle_reader *rle = malloc(sizeof(struct rle_reader));
	rle->vli = vli;
	rle->cnt = 0;
	rle->val = -1;
	return rle;
}

struct rle_writer *rle_writer(struct vli_writer *vli)
{
	struct rle_writer *rle = malloc(sizeof(struct rle_writer));
	rle->vli = vli;
	rle->cnt = 0;
	rle->val = -1;
	return rle;
}

int rle_flush(struct rle_writer *rle)
{
	return rle->cnt = put_vli(rle->vli, rle->cnt, rle->val);
}

void delete_rle_reader(struct rle_reader *rle)
{
	if (rle->cnt > 1)
		fprintf(stderr, "%d values not read.\n", rle->cnt);
	free(rle);
}

void delete_rle_writer(struct rle_writer *rle)
{
	if (rle->cnt > 0)
		fprintf(stderr, "forgot to flush counter for %d values.\n", rle->cnt);
	free(rle);
}

int put_rle(struct rle_writer *rle, int val)
{
	if (rle->cnt < 0)
		return rle->cnt;
	val = !!val;
	if (rle->val < 0)
		return rle->cnt = vli_put_bit(rle->vli, !(rle->val = val));
	if (rle->val != val) {
		rle->cnt = put_vli(rle->vli, rle->cnt, rle->val);
		rle->val = val;
		return rle->cnt;
	}
	rle->cnt++;
	return 0;
}

int get_rle(struct rle_reader *rle)
{
	if (rle->cnt < 0)
		return rle->cnt;
	if ((rle->val < 0 && (rle->val = vli_get_bit(rle->vli)) < 0))
		return rle->val;
	if (rle->cnt)
		rle->cnt--;
	else if ((rle->cnt = get_vli(rle->vli, rle->val ^= 1)) < 0)
		return rle->cnt;
	return rle->val;
}

