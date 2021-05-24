/*
Run length encoding of mostly zero bits

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "vli.h"

struct rle_reader {
	struct vli_reader *vli;
	int cnt;
};

struct rle_writer {
	struct vli_writer *vli;
	int cnt;
};

struct rle_reader *rle_reader(struct vli_reader *vli)
{
	struct rle_reader *rle = malloc(sizeof(struct rle_reader));
	rle->vli = vli;
	rle->cnt = 0;
	return rle;
}

struct rle_writer *rle_writer(struct vli_writer *vli)
{
	struct rle_writer *rle = malloc(sizeof(struct rle_writer));
	rle->vli = vli;
	rle->cnt = 0;
	return rle;
}

int rle_flush(struct rle_writer *rle)
{
	return rle->cnt = put_vli(rle->vli, rle->cnt);
}

int rle_start(struct rle_reader *rle)
{
	rle->cnt = get_vli(rle->vli);
	if (rle->cnt < 0)
		return rle->cnt;
	return 0;
}

void delete_rle_reader(struct rle_reader *rle)
{
	if (rle->cnt > 0)
		fprintf(stderr, "%d zeros not read.\n", rle->cnt);
	free(rle);
}

void delete_rle_writer(struct rle_writer *rle)
{
	if (rle->cnt > 0)
		fprintf(stderr, "forgot to flush counter for %d zeros.\n", rle->cnt);
	free(rle);
}

int put_rle(struct rle_writer *rle, int b)
{
	if (rle->cnt < 0)
		return rle->cnt;
	if (b)
		return rle->cnt = put_vli(rle->vli, rle->cnt);
	rle->cnt++;
	return 0;
}

int get_rle(struct rle_reader *rle)
{
	if (rle->cnt < 0)
		return rle->cnt;
	if (rle->cnt--)
		return 0;
	rle->cnt = get_vli(rle->vli);
	if (rle->cnt < 0)
		return rle->cnt;
	return 1;
}

