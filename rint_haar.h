/*
Reversible integer Haar wavelet

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void rint_haar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)/2*SO] = floorf((in[(i+0)*SI] + in[(i+1)*SI]) / 2.f);
		out[(i+N)/2*SO] = in[(i+0)*SI] - in[(i+1)*SI];
	}
}

void rint_ihaar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)*SO] = in[(i+0)/2*SI] + floorf((in[(i+N)/2*SI] + 1.f) / 2.f);
		out[(i+1)*SO] = in[(i+0)/2*SI] - floorf(in[(i+N)/2*SI] / 2.f);
	}
}

void rint_haar2d(float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int l = N; l >= N0; l /= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				float a = in[(N*(j+0)+i+0)*SI], b = in[(N*(j+0)+i+1)*SI];
				float c = in[(N*(j+1)+i+0)*SI], d = in[(N*(j+1)+i+1)*SI];
				out[(N*(j+0)+i+0)/2*SO] = floorf((floorf((a + b) / 2.f) + floorf((c + d) / 2.f)) / 2.f);
				out[(N*(j+0)+i+l)/2*SO] = floorf((a + b) / 2.f) - floorf((c + d) / 2.f);
				out[(N*(j+l)+i+0)/2*SO] = floorf(((a - b) + (c - d)) / 2.f);
				out[(N*(j+l)+i+l)/2*SO] = (a - b) - (c - d);
			}
		}
		for (int j = 0; j < l / 2; ++j)
			for (int i = 0; i < l / 2; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
	}
}

void rint_ihaar2d(float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int l = N0; l <= N; l *= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				float a = in[(N*(j+0)+i+0)/2*SI], b = in[(N*(j+0)+i+l)/2*SI];
				float c = in[(N*(j+l)+i+0)/2*SI], d = in[(N*(j+l)+i+l)/2*SI];
				out[(N*(j+0)+i+0)*SO] = a + floorf((b + 1.f) / 2.f) + floorf((c + floorf((d + 1.f) / 2.f) + 1.f) / 2.f);
				out[(N*(j+0)+i+1)*SO] = a + floorf((b + 1.f) / 2.f) - floorf((c + floorf((d + 1.f) / 2.f)) / 2.f);
				out[(N*(j+1)+i+0)*SO] = a - floorf(b / 2.f) + floorf((c - floorf(d / 2.f) + 1.f) / 2.f);
				out[(N*(j+1)+i+1)*SO] = a - floorf(b / 2.f) - floorf((c - floorf(d / 2.f)) / 2.f);
			}
		}
		for (int j = 0; j < l; ++j)
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
	}
}

