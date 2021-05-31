/*
Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		float ia = in[(i+0)*SI], ib = in[(i+1)*SI];
		float oa = (ia + ib) / sqrtf(2.f);
		float ob = (ia - ib) / sqrtf(2.f);
		out[(i+0)/2*SO] = oa;
		out[(i+N)/2*SO] = ob;
	}
}

void ihaar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		float ia = in[(i+0)/2*SI], ib = in[(i+N)/2*SI];
		float oa = (ia + ib) / sqrtf(2.f);
		float ob = (ia - ib) / sqrtf(2.f);
		out[(i+0)*SO] = oa;
		out[(i+1)*SO] = ob;
	}
}

