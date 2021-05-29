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
	int acc;
	int cnt;
	int cap;
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
	bits->acc = 0;
	bits->cnt = 0;
	bits->cap = capacity;
	bits->num = 0;
	return bits;
}

int bits_count(struct bits_writer *bits)
{
	return bits->num * 8 + bits->cnt;
}

void close_reader(struct bits_reader *bits)
{
	fclose(bits->file);
	free(bits);
}

void close_writer(struct bits_writer *bits)
{
	if (bits->cnt && bits->acc != fputc(bits->acc, bits->file))
		fprintf(stderr, "could not write to file \"%s\".\n", bits->name);
	fclose(bits->file);
	free(bits);
}

int put_bit(struct bits_writer *bits, int b)
{
	if (bits->cap > 0 && bits->num * 8 + bits->cnt >= bits->cap)
		return -2;
	bits->acc |= !!b << bits->cnt++;
	if (bits->cnt >= 8) {
		bits->cnt -= 8;
		bits->num += 1;
		int c = bits->acc & 255;
		bits->acc >>= 8;
		if (c != fputc(c, bits->file)) {
			fprintf(stderr, "could not write to file \"%s\".\n", bits->name);
			return -1;
		}
	}
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

