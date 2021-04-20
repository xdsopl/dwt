/*
Variable length integer coding

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include "bits.h"

void put_vli(struct bits *bits, int val)
{
	int cnt = 0, top = 1;
	while (top <= val) {
		cnt += 1;
		top = 1 << cnt;
		put_bit(bits, 1);
	}
	put_bit(bits, 0);
	if (cnt > 0) {
		cnt -= 1;
		val -= top/2;
		write_bits(bits, val, cnt);
	}
}

int get_vli(struct bits *bits)
{
	int val = 0, cnt = 0, top = 1;
	while (get_bit(bits)) {
		cnt += 1;
		top = 1 << cnt;
	}
	if (cnt > 0) {
		cnt -= 1;
		read_bits(bits, &val, cnt);
		val += top/2;
	}
	return val;
}

