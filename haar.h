/*
Discrete wavelet transform using the Haar wavelet

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <math.h>

void haar(float *out, float *in, int N, int S)
{
	for (int l = N / 2; l > 0; l /= 2) {
		for (int i = 0; i < l; i++) {
			out[(0 + i) * S] = (in[(i * 2 + 0) * S] + in[(i * 2 + 1) * S]) / sqrtf(2.0f);
			out[(l + i) * S] = (in[(i * 2 + 0) * S] - in[(i * 2 + 1) * S]) / sqrtf(2.0f);
		}
		for (int i = 0; i < l; i++)
			in[i * S] = out[i * S];
	}
}

void ihaar(float *out, float *in, int N, int S)
{
	for (int l = 1; l < N; l *= 2) {
		for (int i = 0; i < l; i++) {
			out[(i * 2 + 0) * S] = (in[(i + 0) * S] + in[(l + i) * S]) / sqrtf(2.0f);
			out[(i * 2 + 1) * S] = (in[(i + 0) * S] - in[(l + i) * S]) / sqrtf(2.0f);
		}
		for (int i = 0; i < 2 * l; i++)
			in[i * S] = out[i * S];
	}
}

void haar2(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; i++)
		haar(out + S * N * i, in + S * N * i, N, S);
	for (int i = 0; i < N * N; i++)
		in[i * S] = out[i * S];
	for (int i = 0; i < N; i++)
		haar(out + S * i, in + S * i, N, S * N);
}

void ihaar2(float *out, float *in, int N, int S)
{
	for (int i = 0; i < N; i++)
		ihaar(out + S * i, in + S * i, N, S * N);
	for (int i = 0; i < N * N; i++)
		in[i * S] = out[i * S];
	for (int i = 0; i < N; i++)
		ihaar(out + S * N * i, in + S * N * i, N, S);
}

void haar2d(float *out, float *in, int N, int S)
{
	for (int l = N / 2; l > 0; l /= 2) {
		for (int j = 0; j < l; j++) {
			for (int i = 0; i < l; i++) {
				float a = in[(N*(2*j+0)+2*i+0)*S], b = in[(N*(2*j+0)+2*i+1)*S];
				float c = in[(N*(2*j+1)+2*i+0)*S], d = in[(N*(2*j+1)+2*i+1)*S];
				out[(N*(j+0)+i+0)*S] = (a + b + c + d) / 2.f;
				out[(N*(j+0)+i+l)*S] = ((a - b) + (c - d)) / 2.f;
				out[(N*(j+l)+i+0)*S] = ((a - c) + (b - d)) / 2.f;
				out[(N*(j+l)+i+l)*S] = ((a - b) - (c - d)) / 2.f;
			}
		}
		for (int j = 0; j < l; j++)
			for (int i = 0; i < l; i++)
				in[(N*j+i)*S] = out[(N*j+i)*S];
	}
}

void ihaar2d(float *out, float *in, int N, int S)
{
	for (int l = 1; l < N; l *= 2) {
		for (int j = 0; j < l; j++) {
			for (int i = 0; i < l; i++) {
				float a = in[(N*(j+0)+i+0)*S], b = in[(N*(j+0)+i+l)*S];
				float c = in[(N*(j+l)+i+0)*S], d = in[(N*(j+l)+i+l)*S];
				out[(N*(2*j+0)+2*i+0)*S] = (a + b + c + d) / 2.f;
				out[(N*(2*j+0)+2*i+1)*S] = ((a - b) + (c - d)) / 2.f;
				out[(N*(2*j+1)+2*i+0)*S] = ((a - c) + (b - d)) / 2.f;
				out[(N*(2*j+1)+2*i+1)*S] = ((a - b) - (c - d)) / 2.f;
			}
		}
		for (int j = 0; j < 2 * l; j++)
			for (int i = 0; i < 2 * l; i++)
				in[(N*j+i)*S] = out[(N*j+i)*S];
	}
}

void haar3(float *out, float *in, int N, int S)
{
	for (int l = N / 2; l > 0; l /= 2) {
		for (int j = 0; j < 2 * l; j++) {
			for (int i = 0; i < l; i++) {
				out[(N*j+0+i)*S] = (in[(N*j+i*2+0)*S] + in[(N*j+i*2+1)*S]) / sqrtf(2.f);
				out[(N*j+l+i)*S] = (in[(N*j+i*2+0)*S] - in[(N*j+i*2+1)*S]) / sqrtf(2.f);
			}
			for (int i = 0; i < 2 * l; i++)
				in[(N*j+i)*S] = out[(N*j+i)*S];
		}
		for (int j = 0; j < 2 * l; j++) {
			for (int i = 0; i < l; i++) {
				out[(j+N*(0+i))*S] = (in[(j+N*(i*2+0))*S] + in[(j+N*(i*2+1))*S]) / sqrtf(2.f);
				out[(j+N*(l+i))*S] = (in[(j+N*(i*2+0))*S] - in[(j+N*(i*2+1))*S]) / sqrtf(2.f);
			}
			for (int i = 0; i < l; i++)
				in[(j+N*i)*S] = out[(j+N*i)*S];
		}
	}
}

void ihaar3(float *out, float *in, int N, int S)
{
	for (int l = 1; l < N; l *= 2) {
		for (int j = 0; j < 2 * l; j++) {
			for (int i = 0; i < l; i++) {
				out[(j+N*(i*2+0))*S] = (in[(j+N*(0+i))*S] + in[(j+N*(l+i))*S]) / sqrtf(2.f);
				out[(j+N*(i*2+1))*S] = (in[(j+N*(0+i))*S] - in[(j+N*(l+i))*S]) / sqrtf(2.f);
			}
			for (int i = 0; i < 2 * l; i++)
				in[(j+N*i)*S] = out[(j+N*i)*S];
		}
		for (int j = 0; j < 2 * l; j++) {
			for (int i = 0; i < l; i++) {
				out[(N*j+i*2+0)*S] = (in[(N*j+0+i)*S] + in[(N*j+l+i)*S]) / sqrtf(2.f);
				out[(N*j+i*2+1)*S] = (in[(N*j+0+i)*S] - in[(N*j+l+i)*S]) / sqrtf(2.f);
			}
			for (int i = 0; i < 2 * l; i++)
				in[(N*j+i)*S] = out[(N*j+i)*S];
		}
	}
}

