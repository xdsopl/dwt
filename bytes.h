/*
Read and write bytes to and from a file

Copyright 2024 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>

struct bytes_reader {
	FILE *file;
	char *name;
};

struct bytes_writer {
	FILE *file;
	char *name;
	int cnt;
	int cap;
};

struct bytes_reader *bytes_reader(char *name)
{
	const char *fname = "/dev/stdin";
	if (name[0] != '-' || name[1])
		fname = name;
	FILE *file = fopen(fname, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read\n", fname);
		return 0;
	}
	struct bytes_reader *bytes = malloc(sizeof(struct bytes_reader));
	bytes->file = file;
	bytes->name = name;
	return bytes;
}

struct bytes_writer *bytes_writer(char *name, int capacity)
{
	const char *fname = "/dev/stdout";
	if (name[0] != '-' || name[1])
		fname = name;
	FILE *file = fopen(fname, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to write\n", fname);
		return 0;
	}
	struct bytes_writer *bytes = malloc(sizeof(struct bytes_writer));
	bytes->file = file;
	bytes->name = name;
	bytes->cnt = 0;
	bytes->cap = capacity;
	return bytes;
}

int bytes_count(struct bytes_writer *bytes)
{
	return bytes->cnt;
}

void close_bytes_reader(struct bytes_reader *bytes)
{
	fclose(bytes->file);
	free(bytes);
}

void close_bytes_writer(struct bytes_writer *bytes)
{
	fclose(bytes->file);
	free(bytes);
}

int put_byte(struct bytes_writer *bytes, int b)
{
	if (bytes->cap > 0 && bytes->cnt >= bytes->cap)
		return -2;
	if (EOF == fputc(b & 255, bytes->file)) {
		fprintf(stderr, "could not write to file \"%s\"\n", bytes->name);
		return -1;
	}
	bytes->cnt += 1;
	return 0;
}

int write_bytes(struct bytes_writer *bytes, int b, int n)
{
	for (int i = 0; i < 8 * n; i += 8) {
		int ret = put_byte(bytes, b >> i);
		if (ret)
			return ret;
	}
	return 0;
}

int get_byte(struct bytes_reader *bytes)
{
	int b = fgetc(bytes->file);
	if (b == EOF) {
		fprintf(stderr, "reached end of file \"%s\"\n", bytes->name);
		return -1;
	}
	return b;
}

int read_bytes(struct bytes_reader *bytes, int *b, int n)
{
	int a = 0;
	for (int i = 0; i < 8 * n; i += 8) {
		int b = get_byte(bytes);
		if (b < 0)
			return b;
		a |= b << i;
	}
	*b = a;
	return 0;
}

