/*
Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar2d(int *out, int *in, int N0, int N, int SO, int SI)
{
	for (int l = N; l >= N0; l /= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				int a = in[(N*(j+0)+i+0)*SI], b = in[(N*(j+0)+i+1)*SI];
				int c = in[(N*(j+1)+i+0)*SI], d = in[(N*(j+1)+i+1)*SI];
				out[(N*(j+0)+i+0)/2*SO] = a + b + c + d;
				out[(N*(j+0)+i+l)/2*SO] = (a - b) + (c - d);
				out[(N*(j+l)+i+0)/2*SO] = (a - c) + (b - d);
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
				out[(N*(j+0)+i+0)*SO] = (a + b + c + d) / 4;
				out[(N*(j+0)+i+1)*SO] = ((a - b) + (c - d)) / 4;
				out[(N*(j+1)+i+0)*SO] = ((a - c) + (b - d)) / 4;
				out[(N*(j+1)+i+1)*SO] = ((a - b) - (c - d)) / 4;
			}
		}
		for (int j = 0; j < l; ++j)
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
	}
}

