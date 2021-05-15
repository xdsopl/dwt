/*
Variable length integer coding

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

int put_vli(struct bits_writer *bits, int val)
{
	int cnt = 0, top = 1;
	while (top <= val) {
		cnt += 1;
		top = 1 << cnt;
		int ret = put_bit(bits, 1);
		if (ret)
			return ret;
	}
	int ret = put_bit(bits, 0);
	if (ret)
		return ret;
	if (cnt > 0) {
		cnt -= 1;
		val -= top/2;
		int ret = write_bits(bits, val, cnt);
		if (ret)
			return ret;
	}
	return 0;
}

int get_vli(struct bits_reader *bits)
{
	int val = 0, cnt = 0, top = 1, ret;
	while ((ret = get_bit(bits)) == 1) {
		cnt += 1;
		top = 1 << cnt;
	}
	if (ret < 0)
		return ret;
	if (cnt > 0) {
		cnt -= 1;
		if ((ret = read_bits(bits, &val, cnt)))
			return ret;
		val += top/2;
	}
	return val;
}

