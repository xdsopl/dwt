/*
Discrete wavelet transform

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void dwt(void (*wavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int l = N; l >= N0; l /= 2) {
		wavelet(out, in, l, SO, SI);
		for (int i = 0; i < l / 2; ++i)
			in[i*SI] = out[i*SO];
	}
}

void idwt(void (*iwavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int l = N0; l <= N; l *= 2) {
		iwavelet(out, in, l, SO, SI);
		for (int i = 0; i < l; ++i)
			in[i*SI] = out[i*SO];
	}
}

void dwt2(void (*wavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int i = 0; i < N; ++i)
		dwt(wavelet, out+SO*N*i, in+SI*N*i, N0, N, SO, SI);
	for (int i = 0; i < N * N; ++i)
		in[i*SI] = out[i*SO];
	for (int i = 0; i < N; ++i)
		dwt(wavelet, out+SO*i, in+SI*i, N0, N, SO*N, SI*N);
}

void idwt2(void (*iwavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int i = 0; i < N; ++i)
		idwt(iwavelet, out+SO*i, in+SI*i, N0, N, SO*N, SI*N);
	for (int i = 0; i < N * N; ++i)
		in[i*SI] = out[i*SO];
	for (int i = 0; i < N; ++i)
		idwt(iwavelet, out+SO*N*i, in+SI*N*i, N0, N, SO, SI);
}

void dwt2d(void (*wavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int l = N; l >= N0; l /= 2) {
		for (int j = 0; j < l; ++j) {
			wavelet(out+SO*N*j, in+SI*N*j, l, SO, SI);
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
		}
		for (int j = 0; j < l; ++j)
			wavelet(out+SO*j, in+SI*j, l, SO*N, SI*N);
		for (int j = 0; j < l / 2; ++j)
			for (int i = 0; i < l / 2; ++i)
				in[(j+N*i)*SI] = out[(j+N*i)*SO];
	}
}

void idwt2d(void (*iwavelet)(float *, float *, int, int, int), float *out, float *in, int N0, int N, int SO, int SI)
{
	for (int l = N0; l <= N; l *= 2) {
		for (int j = 0; j < l; ++j) {
			iwavelet(out+SO*j, in+SI*j, l, SO*N, SI*N);
			for (int i = 0; i < l; ++i)
				in[(j+N*i)*SI] = out[(j+N*i)*SO];
		}
		for (int j = 0; j < l; ++j) {
			iwavelet(out+SO*N*j, in+SI*N*j, l, SO, SI);
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*SI] = out[(N*j+i)*SO];
		}
	}
}

