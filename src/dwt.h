/*
Discrete wavelet transform

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void dwt(void (*wavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	wavelet(out, in, N, SO, SI);
	int K = (N+1)/2;
	for (int i = 0; i < K; ++i)
		in[i*SI] = out[i*SO];
	if (K >= N0)
		dwt(wavelet, out, in, N0, K, SO, SI);
}

void idwt(void (*iwavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	int K = (N+1)/2;
	if (K >= N0)
		idwt(iwavelet, out, in, N0, K, SO, SI);
	iwavelet(out, in, N, SO, SI);
	for (int i = 0; i < N; ++i)
		in[i*SI] = out[i*SO];
}

void dwt2(void (*wavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int W, int H, int SO, int SI)
{
	for (int i = 0; i < H; ++i)
		dwt(wavelet, out+SO*W*i, in+SI*W*i, N0, W, SO, SI);
	for (int i = 0; i < W * H; ++i)
		in[i*SI] = out[i*SO];
	for (int i = 0; i < W; ++i)
		dwt(wavelet, out+SO*i, in+SI*i, N0, H, SO*W, SI*W);
}

void idwt2(void (*iwavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int W, int H, int SO, int SI)
{
	for (int i = 0; i < W; ++i)
		idwt(iwavelet, out+SO*i, in+SI*i, N0, H, SO*W, SI*W);
	for (int i = 0; i < W * H; ++i)
		in[i*SI] = out[i*SO];
	for (int i = 0; i < H; ++i)
		idwt(iwavelet, out+SO*W*i, in+SI*W*i, N0, W, SO, SI);
}

void dwt2d(void (*wavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int W, int H, int SO, int SI, int SW)
{
	for (int j = 0; j < H; ++j) {
		wavelet(out+SO*SW*j, in+SI*SW*j, W, SO, SI);
		for (int i = 0; i < W; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	}
	for (int i = 0; i < W; ++i)
		wavelet(out+SO*i, in+SI*i, H, SO*SW, SI*SW);
	int W2 = (W+1)/2, H2 = (H+1)/2;
	for (int j = 0; j < H2; ++j)
		for (int i = 0; i < W2; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	if (W2 >= N0 && H2 >= N0)
		dwt2d(wavelet, out, in, N0, W2, H2, SO, SI, SW);
}

void idwt2d(void (*iwavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int W, int H, int SO, int SI, int SW)
{
	int W2 = (W+1)/2, H2 = (H+1)/2;
	if (W2 >= N0 && H2 >= N0)
		idwt2d(iwavelet, out, in, N0, W2, H2, SO, SI, SW);
	for (int i = 0; i < W; ++i)
		iwavelet(out+SO*i, in+SI*i, H, SO*SW, SI*SW);
	for (int j = 0; j < H; ++j)
		for (int i = 0; i < W; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	for (int j = 0; j < H; ++j) {
		iwavelet(out+SO*SW*j, in+SI*SW*j, W, SO, SI);
		for (int i = 0; i < W; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	}
}

