/*
Run length encoding of mostly zero bits

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "vli.h"
#include "bits.h"

struct rle_reader {
	struct bits_reader *bits;
	int cnt;
};

struct rle_writer {
	struct bits_writer *bits;
	int cnt;
};

struct rle_reader *rle_reader(struct bits_reader *bits)
{
	struct rle_reader *rle = malloc(sizeof(struct rle_reader));
	rle->bits = bits;
	rle->cnt = 0;
	return rle;
}

struct rle_writer *rle_writer(struct bits_writer *bits)
{
	struct rle_writer *rle = malloc(sizeof(struct rle_writer));
	rle->bits = bits;
	rle->cnt = 0;
	return rle;
}

int rle_flush(struct rle_writer *rle)
{
	return rle->cnt = put_vli(rle->bits, rle->cnt);
}

int rle_start(struct rle_reader *rle)
{
	rle->cnt = get_vli(rle->bits);
	if (rle->cnt < 0)
		return rle->cnt;
	return 0;
}

void delete_reader(struct rle_reader *rle)
{
	if (rle->cnt > 0)
		fprintf(stderr, "%d zeros not read.\n", rle->cnt);
	free(rle);
}

void delete_writer(struct rle_writer *rle)
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
		return rle->cnt = put_vli(rle->bits, rle->cnt);
	rle->cnt++;
	return 0;
}

int get_rle(struct rle_reader *rle)
{
	if (rle->cnt < 0)
		return rle->cnt;
	if (rle->cnt--)
		return 0;
	rle->cnt = get_vli(rle->bits);
	if (rle->cnt < 0)
		return rle->cnt;
	return 1;
}

