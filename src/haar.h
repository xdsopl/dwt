/*
Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0, M = N&~1, K = N+(N&1); i < M; i += 2) {
		float ia = in[(i+0)*SI], ib = in[(i+1)*SI];
		float oa = (ia + ib) / sqrtf(2.f);
		float ob = (ia - ib) / sqrtf(2.f);
		out[(i+0)/2*SO] = oa;
		out[(i+K)/2*SO] = ob;
	}
	if (N&1)
		out[(N-1)/2*SO] = in[(N-1)*SI];
}

void ihaar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0, M = N&~1, K = N+(N&1); i < M; i += 2) {
		float ia = in[(i+0)/2*SI], ib = in[(i+K)/2*SI];
		float oa = (ia + ib) / sqrtf(2.f);
		float ob = (ia - ib) / sqrtf(2.f);
		out[(i+0)*SO] = oa;
		out[(i+1)*SO] = ob;
	}
	if (N&1)
		out[(N-1)*SO] = in[(N-1)/2*SI];
}

