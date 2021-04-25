/*
Discrete wavelet transform

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void dwt(void (*wavelet)(float *, float *, int, int), float *out, float *in, int N0, int N, int S)
{
	for (int l = N; l >= N0; l /= 2) {
		wavelet(out, in, l, S);
		for (int i = 0; i < l / 2; ++i)
			in[i*S] = out[i*S];
	}
}

void idwt(void (*iwavelet)(float *, float *, int, int), float *out, float *in, int N0, int N, int S)
{
	for (int l = N0; l <= N; l *= 2) {
		iwavelet(out, in, l, S);
		for (int i = 0; i < l; ++i)
			in[i*S] = out[i*S];
	}
}

void dwt2(void (*wavelet)(float *, float *, int, int), float *out, float *in, int N0, int N, int S)
{
	for (int i = 0; i < N; ++i)
		dwt(wavelet, out+S*N*i, in+S*N*i, N0, N, S);
	for (int i = 0; i < N * N; ++i)
		in[i*S] = out[i*S];
	for (int i = 0; i < N; ++i)
		dwt(wavelet, out+S*i, in+S*i, N0, N, S*N);
}

void idwt2(void (*iwavelet)(float *, float *, int, int), float *out, float *in, int N0, int N, int S)
{
	for (int i = 0; i < N; ++i)
		idwt(iwavelet, out+S*i, in+S*i, N0, N, S*N);
	for (int i = 0; i < N * N; ++i)
		in[i*S] = out[i*S];
	for (int i = 0; i < N; ++i)
		idwt(iwavelet, out+S*N*i, in+S*N*i, N0, N, S);
}

void dwt2d(void (*wavelet)(float *, float *, int, int), float *out, float *in, int N0, int N, int S)
{
	for (int l = N; l >= N0; l /= 2) {
		for (int j = 0; j < l; ++j) {
			wavelet(out+S*N*j, in+S*N*j, l, S);
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
		}
		for (int j = 0; j < l; ++j) {
			wavelet(out+S*j, in+S*j, l, S*N);
			for (int i = 0; i < l / 2; ++i)
				in[(j+N*i)*S] = out[(j+N*i)*S];
		}
	}
}

void idwt2d(void (*iwavelet)(float *, float *, int, int), float *out, float *in, int N0, int N, int S)
{
	for (int l = N0; l <= N; l *= 2) {
		for (int j = 0; j < l; ++j) {
			iwavelet(out+S*j, in+S*j, l, S*N);
			for (int i = 0; i < l; ++i)
				in[(j+N*i)*S] = out[(j+N*i)*S];
		}
		for (int j = 0; j < l; ++j) {
			iwavelet(out+S*N*j, in+S*N*j, l, S);
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
		}
	}
}

