/*
Read and write bits to and from a file

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>

struct bits_reader {
	FILE *file;
	char *name;
	int acc;
	int cnt;
};

struct bits_writer {
	FILE *file;
	char *name;
	char *buf;
	int cap;
	int cnt;
	int rem;
	int num;
};

struct bits_reader *bits_reader(char *name)
{
	FILE *file = fopen(name, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read.\n", name);
		return 0;
	}
	struct bits_reader *bits = malloc(sizeof(struct bits_reader));
	bits->file = file;
	bits->name = name;
	bits->acc = 0;
	bits->cnt = 0;
	return bits;
}

struct bits_writer *bits_writer(char *name, int capacity)
{
	FILE *file = fopen(name, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to write.\n", name);
		return 0;
	}
	struct bits_writer *bits = malloc(sizeof(struct bits_writer));
	bits->file = file;
	bits->name = name;
	bits->cap = (capacity + 7) / 8;
	bits->buf = malloc(bits->cap);
	for (int i = 0; i < bits->cap; ++i)
		bits->buf[i] = 0;
	bits->cnt = 0;
	bits->rem = 0;
	bits->num = 0;
	return bits;
}

int bits_count(struct bits_writer *bits)
{
	return bits->num * 8 + bits->cnt;
}

void bits_flush(struct bits_writer *bits)
{
	for (int i = 0; i < bits->cnt/8; ++i) {
		int c = bits->buf[i] & 255;
		bits->buf[i] = 0;
		bits->num += 1;
		if (c != fputc(c, bits->file))
			fprintf(stderr, "could not write to file \"%s\".\n", bits->name);
	}
	bits->rem = bits->cnt % 8;
	if (bits->rem && bits->cnt/8) {
		bits->buf[0] = bits->buf[bits->cnt/8];
		bits->buf[bits->cnt/8] = 0;
	}
	bits->cnt = bits->rem;
}

void bits_discard(struct bits_writer *bits)
{
	bits->buf[0] &= (1 << bits->rem) - 1;
	for (int i = 1; i < (bits->cnt+7)/8; ++i)
		bits->buf[i] = 0;
	bits->cnt = bits->rem;
}

void close_reader(struct bits_reader *bits)
{
	fclose(bits->file);
	free(bits);
}

void close_writer(struct bits_writer *bits)
{
	bits_flush(bits);
	int c = bits->buf[0] & 255;
	if (bits->rem && c != fputc(c, bits->file))
		fprintf(stderr, "could not write to file \"%s\".\n", bits->name);
	fclose(bits->file);
	free(bits->buf);
	free(bits);
}

int put_bit(struct bits_writer *bits, int b)
{
	if (bits->cnt >= bits->cap * 8)
		return 1;
	bits->buf[bits->cnt/8] |= !!b << bits->cnt%8;
	bits->cnt += 1;
	return 0;
}

int write_bits(struct bits_writer *bits, int b, int n)
{
	for (int i = 0; i < n; ++i) {
		int ret = put_bit(bits, (b>>i)&1);
		if (ret)
			return ret;
	}
	return 0;
}

int get_bit(struct bits_reader *bits)
{
	if (!bits->cnt) {
		int c = fgetc(bits->file);
		if (c == EOF) {
			fprintf(stderr, "could not read from file \"%s\".\n", bits->name);
			return -1;
		}
		bits->acc = c;
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

