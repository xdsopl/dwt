/*
Reversible integer Cohen–Daubechies–Feauveau 5/3 wavelet

Copied from cdf97.h, using A = -1/2, B = 1/4, C = 0, D = 0 and E = 1

Copyright 2024 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void cdf53(float *out, float *in, int N, int SO, int SI, int CH)
{
	float	A = -0.5f,
		B = 0.25f,
		C = 0,
		D = 0,
		E = 1;

	int M = N&~1, K = N+(N&1);
	for (int i = 1; i < N-1; i += 2)
		for (int c = 0; c < CH; ++c)
			in[i*SI+c] += A * (in[(i-1)*SI+c] + in[(i+1)*SI+c]);
	if (!(N&1))
		for (int c = 0; c < CH; ++c)
			in[(N-1)*SI+c] += A * (2.f * in[(N-2)*SI+c]);

	for (int c = 0; c < CH; ++c)
		in[c] += B * (2.f * in[SI+c]);
	for (int i = 2; i < M; i += 2)
		for (int c = 0; c < CH; ++c)
			in[i*SI+c] += B * (in[(i-1)*SI+c] + in[(i+1)*SI+c]);

	for (int i = 1; i < N-1; i += 2)
		for (int c = 0; c < CH; ++c)
			in[i*SI+c] += C * (in[(i-1)*SI+c] + in[(i+1)*SI+c]);
	if (!(N&1))
		for (int c = 0; c < CH; ++c)
			in[(N-1)*SI+c] += C * (2.f * in[(N-2)*SI+c]);

	for (int c = 0; c < CH; ++c)
		in[c] += D * (2.f * in[SI+c]);
	for (int i = 2; i < M; i += 2)
		for (int c = 0; c < CH; ++c)
			in[i*SI+c] += D * (in[(i-1)*SI+c] + in[(i+1)*SI+c]);

	for (int i = 0; i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			out[(i+0)/2*SO+c] = in[(i+0)*SI+c] * E;
			out[(i+K)/2*SO+c] = in[(i+1)*SI+c] / E;
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)/2*SO+c] = in[(N-1)*SI+c] * E;
}

void icdf53(float *out, float *in, int N, int SO, int SI, int CH)
{
	float	A = -0.5f,
		B = 0.25f,
		C = 0,
		D = 0,
		E = 1;

	int M = N&~1, K = N+(N&1);
	for (int i = 0; i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			out[(i+0)*SO+c] = in[(i+0)/2*SI+c] / E;
			out[(i+1)*SO+c] = in[(i+K)/2*SI+c] * E;
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] = in[(N-1)/2*SI+c] / E;
	for (int c = 0; c < CH; ++c)
		out[c] -= D * (2.f * out[SO+c]);
	for (int i = 2; i < M; i += 2)
		for (int c = 0; c < CH; ++c)
			out[i*SO+c] -= D * (out[(i-1)*SO+c] + out[(i+1)*SO+c]);

	for (int i = 1; i < N-1; i += 2)
		for (int c = 0; c < CH; ++c)
			out[i*SO+c] -= C * (out[(i-1)*SO+c] + out[(i+1)*SO+c]);
	if (!(N&1))
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] -= C * (2.f * out[(N-2)*SO+c]);

	for (int c = 0; c < CH; ++c)
		out[c] -= B * (2.f * out[SO+c]);
	for (int i = 2; i < M; i += 2)
		for (int c = 0; c < CH; ++c)
			out[i*SO+c] -= B * (out[(i-1)*SO+c] + out[(i+1)*SO+c]);

	for (int i = 1; i < N-1; i += 2)
		for (int c = 0; c < CH; ++c)
			out[i*SO+c] -= A * (out[(i-1)*SO+c] + out[(i+1)*SO+c]);
	if (!(N&1))
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] -= A * (2.f * out[(N-2)*SO+c]);
}

