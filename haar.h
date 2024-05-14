/*
Reversible integer Haar wavelet

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void haar(int *out, int *in, int N, int SO, int SI, int CH)
{
	for (int i = 0, M = N&~1, K = N+(N&1); i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			int ia = in[(i+0)*SI+c];
			int ib = in[(i+1)*SI+c];
			int ob = ia - ib;
			int oa = ib + (ob + 1024) / 2 - 512;
			out[(i+0)/2*SO+c] = oa;
			out[(i+K)/2*SO+c] = ob;
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)/2*SO+c] = in[(N-1)*SI+c];
}

void ihaar(int *out, int *in, int N, int SO, int SI, int CH)
{
	for (int i = 0, M = N&~1, K = N+(N&1); i < M; i += 2) {
		for (int c = 0; c < CH; ++c) {
			int ia = in[(i+0)/2*SI+c];
			int ib = in[(i+K)/2*SI+c];
			int ob = ia - (ib + 1024) / 2 + 512;
			int oa = ib + ob;
			out[(i+0)*SO+c] = oa;
			out[(i+1)*SO+c] = ob;
		}
	}
	if (N&1)
		for (int c = 0; c < CH; ++c)
			out[(N-1)*SO+c] = in[(N-1)/2*SI+c];
}

