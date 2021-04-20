/*
Discrete wavelet transform using the Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar(float *out, float *in, int N, int S)
{
	for (int l = N / 2; l > 0; l /= 2) {
		for (int i = 0; i < l; i++) {
			out[(0 + i) * S] = (in[(i * 2 + 0) * S] + in[(i * 2 + 1) * S]) / sqrtf(2.0f);
			out[(l + i) * S] = (in[(i * 2 + 0) * S] - in[(i * 2 + 1) * S]) / sqrtf(2.0f);
		}
		for (int i = 0; i < 2 * l; i++)
			in[i * S] = out[i * S];
	}
}

void ihaar(float *out, float *in, int N, int S)
{
	for (int l = 1; l < N; l *= 2) {
		for (int i = 0; i < l; i++) {
			out[(i * 2 + 0) * S] = (in[(i + 0) * S] + in[(l + i) * S]) / sqrtf(2.0f);
			out[(i * 2 + 1) * S] = (in[(i + 0) * S] - in[(l + i) * S]) / sqrtf(2.0f);
		}
		for (int i = 0; i < 2 * l; i++)
			in[i * S] = out[i * S];
	}
}

