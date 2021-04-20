/*
Read and write bits to and from a file

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>

struct bits {
	FILE *file;
	char *name;
	int acc;
	int cnt;
};

struct bits *new_bits(FILE *file, char *name)
{
	struct bits *bits = malloc(sizeof(struct bits));
	bits->file = file;
	bits->name = name;
	bits->acc = 0;
	bits->cnt = 0;
	return bits;
}

struct bits *bits_reader(char *name)
{
	FILE *file = fopen(name, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read.\n", name);
		return 0;
	}
	return new_bits(file, name);
}

struct bits *bits_writer(char *name)
{
	FILE *file = fopen(name, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to write.\n", name);
		return 0;
	}
	return new_bits(file, name);
}

void close_reader(struct bits *bits)
{
	fclose(bits->file);
	free(bits);
}

void close_writer(struct bits *bits)
{
	if (bits->cnt && bits->acc != fputc(bits->acc, bits->file))
		fprintf(stderr, "could not write to file \"%s\".\n", bits->name);
	close_reader(bits);
}

void put_bit(struct bits *bits, int b)
{
	bits->acc |= !!b << bits->cnt++;
	if (bits->cnt >= 8) {
		bits->cnt -= 8;
		int c = bits->acc & 255;
		bits->acc >>= 8;
		if (c != fputc(c, bits->file))
			fprintf(stderr, "could not write to file \"%s\".\n", bits->name);
	}
}

void write_bits(struct bits *bits, int b, int n)
{
	for (int i = 0; i < n; ++i)
		put_bit(bits, (b>>i)&1);
}

int get_bit(struct bits *bits)
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

void read_bits(struct bits *bits, int *b, int n)
{
	int a = 0;
	for (int i = 0; i < n; ++i)
		a |= get_bit(bits) << i;
	*b = a;
}

