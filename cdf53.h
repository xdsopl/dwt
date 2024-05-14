/*
Reversible integer Cohen–Daubechies–Feauveau 5/3 wavelet

Copyright 2024 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void cdf53(float *out, float *in, int N, int SO, int SI, int CH)
{
	int M = N&~1, K = N+(N&1);
	for (int i = 1; i < N-1; i += 2)
		for (int c = 0; c < CH; ++c)
			in[i*SI+c] -= truncf((in[(i-1)*SI+c] + in[(i+1)*SI+c]) / 2.f);
	if (!(N&1))
		for (int c = 0; c < CH; ++c)
			in[(N-1)*SI+c] -= in[(N-2)*SI+c];

	for (int c = 0; c < CH; ++c)
		in[c] += truncf((in[SI+c]) / 2.f);
	for (int i = 2; i < M; i += 2)
		for (int c = 0; c < CH; ++c)
			in[i*SI+c] += truncf((in[(i-1)*SI+c] + in[(i+1)*SI+c]) / 4.f);

	for (int i = 0; i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			out[(i+0)/2*SO+c] = in[(i+0)*SI+c];
			out[(i+K)/2*SO+c] = in[(i+1)*SI+c];
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)/2*SO+c] = in[(N-1)*SI+c];
}

void icdf53(float *out, float *in, int N, int SO, int SI, int CH)
{
	int M = N&~1, K = N+(N&1);
	for (int i = 0; i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			out[(i+0)*SO+c] = in[(i+0)/2*SI+c];
			out[(i+1)*SO+c] = in[(i+K)/2*SI+c];
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] = in[(N-1)/2*SI+c];

	for (int c = 0; c < CH; ++c)
		out[c] -= truncf((out[SO+c]) / 2.f);
	for (int i = 2; i < M; i += 2)
		for (int c = 0; c < CH; ++c)
			out[i*SO+c] -= truncf((out[(i-1)*SO+c] + out[(i+1)*SO+c]) / 4.f);

	for (int i = 1; i < N-1; i += 2)
		for (int c = 0; c < CH; ++c)
			out[i*SO+c] += truncf((out[(i-1)*SO+c] + out[(i+1)*SO+c]) / 2.f);
	if (!(N&1))
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] += out[(N-2)*SO+c];
}

