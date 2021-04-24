/*
Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)/2*S] = (in[(i+0)*S] + in[(i+1)*S]) / sqrtf(2.f);
		out[(i+N)/2*S] = (in[(i+0)*S] - in[(i+1)*S]) / sqrtf(2.f);
	}
}

void ihaar(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)*S] = (in[(i+0)/2*S] + in[(i+N)/2*S]) / sqrtf(2.f);
		out[(i+1)*S] = (in[(i+0)/2*S] - in[(i+N)/2*S]) / sqrtf(2.f);
	}
}

void haar2d(float *out, float *in, int N, int S)
{
	for (int l = N; l >= 2; l /= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				float a = in[(N*(j+0)+i+0)*S], b = in[(N*(j+0)+i+1)*S];
				float c = in[(N*(j+1)+i+0)*S], d = in[(N*(j+1)+i+1)*S];
				out[(N*(j+0)+i+0)/2*S] = (a + b + c + d) / 2.f;
				out[(N*(j+0)+i+l)/2*S] = ((a - b) + (c - d)) / 2.f;
				out[(N*(j+l)+i+0)/2*S] = ((a - c) + (b - d)) / 2.f;
				out[(N*(j+l)+i+l)/2*S] = ((a - b) - (c - d)) / 2.f;
			}
		}
		for (int j = 0; j < l / 2; ++j)
			for (int i = 0; i < l / 2; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
	}
}

void ihaar2d(float *out, float *in, int N, int S)
{
	for (int l = 2; l <= N; l *= 2) {
		for (int j = 0; j < l; j += 2) {
			for (int i = 0; i < l; i += 2) {
				float a = in[(N*(j+0)+i+0)/2*S], b = in[(N*(j+0)+i+l)/2*S];
				float c = in[(N*(j+l)+i+0)/2*S], d = in[(N*(j+l)+i+l)/2*S];
				out[(N*(j+0)+i+0)*S] = (a + b + c + d) / 2.f;
				out[(N*(j+0)+i+1)*S] = ((a - b) + (c - d)) / 2.f;
				out[(N*(j+1)+i+0)*S] = ((a - c) + (b - d)) / 2.f;
				out[(N*(j+1)+i+1)*S] = ((a - b) - (c - d)) / 2.f;
			}
		}
		for (int j = 0; j < l; ++j)
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
	}
}

