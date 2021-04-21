/*
Hilbert curve

Code below based on:
https://en.wikipedia.org/wiki/Hilbert_curve

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

int hilbert(int n, int d)
{
	int x = 0, y = 0;
	for (int s = 1; s < n; s *= 2, d /= 4) {
		int rx = (d/2) & 1;
		int ry = (d^rx) & 1;
		if (ry == 0) {
			if (rx == 1) {
				x = s-1 - x;
				y = s-1 - y;
			}
			x ^= y;
			y ^= x;
			x ^= y;
		}
		x += s * rx;
		y += s * ry;
	}
	return n * y + x;
}

