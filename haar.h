/*
Haar wavelet

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

int half(int x)
{
	int bias = 1 << (sizeof(int) * 8 - 2);
	return (x + bias) / 2 - bias / 2;
}

void haar(int *out, int *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)/2*SO] = half(in[(i+0)*SI] + in[(i+1)*SI]);
		out[(i+N)/2*SO] = in[(i+0)*SI] - in[(i+1)*SI];
	}
}

void ihaar(int *out, int *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)*SO] = in[(i+0)/2*SI] + half(in[(i+N)/2*SI] + 1);
		out[(i+1)*SO] = in[(i+0)/2*SI] - half(in[(i+N)/2*SI]);
	}
}

void haar2d(int *out, int *in, int N0, int N, int SO, int SI)
{
	for (int l = N; l >= N0; l /= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				int a = in[(N*(j+0)+i+0)*SI], b = in[(N*(j+0)+i+1)*SI];
				int c = in[(N*(j+1)+i+0)*SI], d = in[(N*(j+1)+i+1)*SI];
				out[(N*(j+0)+i+0)/2*SO] = half(half(a + b) + half(c + d));
				out[(N*(j+0)+i+l)/2*SO] = half(a + b) - half(c + d);
				out[(N*(j+l)+i+0)/2*SO] = half((a - b) + (c - d));
				out[(N*(j+l)+i+l)/2*SO] = (a - b) - (c - d);
			}
		}
		for (int j = 0; j < l / 2; ++j)
			for (int i = 0; i < l / 2; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
	}
}

void ihaar2d(int *out, int *in, int N0, int N, int SO, int SI)
{
	for (int l = N0; l <= N; l *= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				int a = in[(N*(j+0)+i+0)/2*SI], b = in[(N*(j+0)+i+l)/2*SI];
				int c = in[(N*(j+l)+i+0)/2*SI], d = in[(N*(j+l)+i+l)/2*SI];
				out[(N*(j+0)+i+0)*SO] = a + half(b + 1) + half(c + half(d + 1) + 1);
				out[(N*(j+0)+i+1)*SO] = a + half(b + 1) - half(c + half(d + 1));
				out[(N*(j+1)+i+0)*SO] = a - half(b) + half(c - half(d) + 1);
				out[(N*(j+1)+i+1)*SO] = a - half(b) - half(c - half(d));
			}
		}
		for (int j = 0; j < l; ++j)
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
	}
}

