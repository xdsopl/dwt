/*
Reversible integer Haar wavelet

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void rint_haar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		float ia = in[(i+0)*SI], ib = in[(i+1)*SI];
		float ob = ia - ib;
		float oa = ib + floorf(ob / 2.f);
		out[(i+0)/2*SO] = oa;
		out[(i+N)/2*SO] = ob;
	}
}

void rint_ihaar(float *out, float *in, int N, int SO, int SI)
{
	for (int i = 0; i < N; i += 2) {
		float ia = in[(i+0)/2*SI], ib = in[(i+N)/2*SI];
		float ob = ia - floorf(ib / 2.f);
		float oa = ib + ob;
		out[(i+0)*SO] = oa;
		out[(i+1)*SO] = ob;
	}
}

