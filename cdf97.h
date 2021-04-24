/*
Cohen–Daubechies–Feauveau 9/7 wavelet

Code and coefficients below based on paper:
Factoring wavelet transforms into lifting steps
by Ingrid Daubechies and Wim Sweldens - 1996

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void cdf97(float *out, float *in, int N, int S)
{
	float	a = -1.586134342f,
		b = -0.05298011854f,
		c = 0.8829110762f,
		d = 0.4435068522f,
		e = 1.149604398f;

	for (int i = 0; i < N-2; i += 2)
		in[(i+1)*S] += a * (in[(i+0)*S] + in[(i+2)*S]);
	in[(N-1)*S] += a * (2.f * in[(N-2)*S]);

	in[0] += b * (2.f * in[S]);
	for (int i = 2; i < N; i += 2)
		in[(i+0)*S] += b * (in[(i+1)*S] + in[(i-1)*S]);

	for (int i = 0; i < N-2; i += 2)
		in[(i+1)*S] += c * (in[(i+0)*S] + in[(i+2)*S]);
	in[(N-1)*S] += c * (2.f * in[(N-2)*S]);

	in[0] += d * (2.f * in[S]);
	for (int i = 2; i < N; i += 2)
		in[(i+0)*S] += d * (in[(i+1)*S] + in[(i-1)*S]);

	for (int i = 0; i < N; i += 2) {
		out[(i+0)/2*S] = in[(i+0)*S] * e;
		out[(i+N)/2*S] = in[(i+1)*S] / e;
	}
}

void icdf97(float *out, float *in, int N, int S)
{
	float	a = -1.586134342f,
		b = -0.05298011854f,
		c = 0.8829110762f,
		d = 0.4435068522f,
		e = 1.149604398f;

	for (int i = 0; i < N; i += 2) {
		out[(i+0)*S] = in[(i+0)/2*S] / e;
		out[(i+1)*S] = in[(i+N)/2*S] * e;
	}
	out[0] -= d * (2.f * out[S]);
	for (int i = 2; i < N; i += 2)
		out[(i+0)*S] -= d * (out[(i+1)*S] + out[(i-1)*S]);

	for (int i = 0; i < N-2; i += 2)
		out[(i+1)*S] -= c * (out[(i+0)*S] + out[(i+2)*S]);
	out[(N-1)*S] -= c * (2.f * out[(N-2)*S]);

	out[0] -= b * (2.f * out[S]);
	for (int i = 2; i < N; i += 2)
		out[(i+0)*S] -= b * (out[(i+1)*S] + out[(i-1)*S]);

	for (int i = 0; i < N-2; i += 2)
		out[(i+1)*S] -= a * (out[(i+0)*S] + out[(i+2)*S]);
	out[(N-1)*S] -= a * (2.f * out[(N-2)*S]);
}

