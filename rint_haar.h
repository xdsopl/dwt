/*
Reversible integer Haar wavelet

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void rint_haar(float *out, float *in, int N, int SO, int SI, int CH)
{
	for (int i = 0, M = N&~1, K = N+(N&1); i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			float ia = in[(i+0)*SI+c];
			float ib = in[(i+1)*SI+c];
			float ob = ia - ib;
			float oa = ib + floorf(ob / 2.f);
			out[(i+0)/2*SO+c] = oa;
			out[(i+K)/2*SO+c] = ob;
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)/2*SO+c] = in[(N-1)*SI+c];
}

void rint_ihaar(float *out, float *in, int N, int SO, int SI, int CH)
{
	for (int i = 0, M = N&~1, K = N+(N&1); i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			float ia = in[(i+0)/2*SI+c];
			float ib = in[(i+K)/2*SI+c];
			float ob = ia - floorf(ib / 2.f);
			float oa = ib + ob;
			out[(i+0)*SO+c] = oa;
			out[(i+1)*SO+c] = ob;
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] = in[(N-1)/2*SI+c];
}

