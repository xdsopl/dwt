/*
Some utils

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

int ilog2(int x)
{
	int l = -1;
	for (; x > 0; x /= 2)
		++l;
	return l;
}

int lengths_helper(int *pixels, int *widths, int *heights, int W, int H, int N0)
{
	int level = 0, W2 = (W + 1) / 2, H2 = (H + 1) / 2;
	if (W2 >= N0 && H2 >= N0)
		level = lengths_helper(pixels, widths, heights, W2, H2, N0);
	widths[level] = W2;
	heights[level] = H2;
	pixels[level] = W2 * H2;
	return level + 1;
}

int compute_lengths(int *lengths, int *pixels, int *widths, int *heights, int W, int H, int N0)
{
	int levels = lengths_helper(pixels, widths, heights, W, H, N0);
	widths[levels] = W;
	heights[levels] = H;
	pixels[levels] = W * H;
	for (int l = 0; l <= levels; ++l) {
		int w = 1 << (ilog2(widths[l] - 1) + 1);
		int h = 1 << (ilog2(heights[l] - 1) + 1);
		lengths[l] = w > h ? w : h;
	}
	return levels;
}

