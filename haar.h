/*
Discrete wavelet transform using the Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar1(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)/2*S] = (in[(i+0)*S] + in[(i+1)*S]) / sqrtf(2.f);
		out[(i+N)/2*S] = (in[(i+0)*S] - in[(i+1)*S]) / sqrtf(2.f);
	}
}

void ihaar1(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; i += 2) {
		out[(i+0)*S] = (in[(i+0)/2*S] + in[(i+N)/2*S]) / sqrtf(2.f);
		out[(i+1)*S] = (in[(i+0)/2*S] - in[(i+N)/2*S]) / sqrtf(2.f);
	}
}

void haar(float *out, float *in, int N, int S)
{
	for (int l = N; l >= 2; l /= 2) {
		haar1(out, in, l, S);
		for (int i = 0; i < l / 2; ++i)
			in[i*S] = out[i*S];
	}
}

void ihaar(float *out, float *in, int N, int S)
{
	for (int l = 2; l <= N; l *= 2) {
		ihaar1(out, in, l, S);
		for (int i = 0; i < l; ++i)
			in[i*S] = out[i*S];
	}
}

void haar2(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; ++i)
		haar(out+S*N*i, in+S*N*i, N, S);
	for (int i = 0; i < N * N; ++i)
		in[i*S] = out[i*S];
	for (int i = 0; i < N; ++i)
		haar(out+S*i, in+S*i, N, S*N);
}

void ihaar2(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; ++i)
		ihaar(out+S*i, in+S*i, N, S*N);
	for (int i = 0; i < N * N; ++i)
		in[i*S] = out[i*S];
	for (int i = 0; i < N; ++i)
		ihaar(out+S*N*i, in+S*N*i, N, S);
}

void haar2d(float *out, float *in, int N, int S)
{
	for (int l = N / 2; l > 0; l /= 2) {
		for (int j = 0; j < l; ++j) {
			for (int i = 0; i < l; ++i) {
				float a = in[(N*(2*j+0)+2*i+0)*S], b = in[(N*(2*j+0)+2*i+1)*S];
				float c = in[(N*(2*j+1)+2*i+0)*S], d = in[(N*(2*j+1)+2*i+1)*S];
				out[(N*(j+0)+i+0)*S] = (a + b + c + d) / 2.f;
				out[(N*(j+0)+i+l)*S] = ((a - b) + (c - d)) / 2.f;
				out[(N*(j+l)+i+0)*S] = ((a - c) + (b - d)) / 2.f;
				out[(N*(j+l)+i+l)*S] = ((a - b) - (c - d)) / 2.f;
			}
		}
		for (int j = 0; j < l; ++j)
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
	}
}

void ihaar2d(float *out, float *in, int N, int S)
{
	for (int l = 1; l < N; l *= 2) {
		for (int j = 0; j < l; ++j) {
			for (int i = 0; i < l; ++i) {
				float a = in[(N*(j+0)+i+0)*S], b = in[(N*(j+0)+i+l)*S];
				float c = in[(N*(j+l)+i+0)*S], d = in[(N*(j+l)+i+l)*S];
				out[(N*(2*j+0)+2*i+0)*S] = (a + b + c + d) / 2.f;
				out[(N*(2*j+0)+2*i+1)*S] = ((a - b) + (c - d)) / 2.f;
				out[(N*(2*j+1)+2*i+0)*S] = ((a - c) + (b - d)) / 2.f;
				out[(N*(2*j+1)+2*i+1)*S] = ((a - b) - (c - d)) / 2.f;
			}
		}
		for (int j = 0; j < 2 * l; ++j)
			for (int i = 0; i < 2 * l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
	}
}

void haar3(float *out, float *in, int N, int S)
{
	for (int l = N; l >= 2; l /= 2) {
		for (int j = 0; j < l; ++j) {
			haar1(out+S*N*j, in+S*N*j, l, S);
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
		}
		for (int j = 0; j < l; ++j) {
			haar1(out+S*j, in+S*j, l, S*N);
			for (int i = 0; i < l / 2; ++i)
				in[(j+N*i)*S] = out[(j+N*i)*S];
		}
	}
}

void ihaar3(float *out, float *in, int N, int S)
{
	for (int l = 2; l <= N; l *= 2) {
		for (int j = 0; j < l; ++j) {
			ihaar1(out+S*j, in+S*j, l, S*N);
			for (int i = 0; i < l; ++i)
				in[(j+N*i)*S] = out[(j+N*i)*S];
		}
		for (int j = 0; j < l; ++j) {
			ihaar1(out+S*N*j, in+S*N*j, l, S);
			for (int i = 0; i < l; ++i)
				in[(N*j+i)*S] = out[(N*j+i)*S];
		}
	}
}

