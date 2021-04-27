/*
Cohen–Daubechies–Feauveau 9/7 wavelet

Code and coefficients below based on paper:
Factoring wavelet transforms into lifting steps
by Ingrid Daubechies and Wim Sweldens - 1996

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void cdf97(float *out, float *in, int N, int SO, int SI)
{
	float	a = -1.586134342f,
		b = -0.05298011854f,
		c = 0.8829110762f,
		d = 0.4435068522f,
		e = 1.149604398f;

	for (int i = 1; i < N-1; i += 2)
		in[i*SI] += a * (in[(i-1)*SI] + in[(i+1)*SI]);
	in[(N-1)*SI] += a * (2.f * in[(N-2)*SI]);

	in[0] += b * (2.f * in[SI]);
	for (int i = 2; i < N; i += 2)
		in[i*SI] += b * (in[(i-1)*SI] + in[(i+1)*SI]);

	for (int i = 1; i < N-1; i += 2)
		in[i*SI] += c * (in[(i-1)*SI] + in[(i+1)*SI]);
	in[(N-1)*SI] += c * (2.f * in[(N-2)*SI]);

	in[0] += d * (2.f * in[SI]);
	for (int i = 2; i < N; i += 2)
		in[i*SI] += d * (in[(i-1)*SI] + in[(i+1)*SI]);

	for (int i = 0; i < N; i += 2) {
		out[(i+0)/2*SO] = in[(i+0)*SI] * e;
		out[(i+N)/2*SO] = in[(i+1)*SI] / e;
	}
}

void icdf97(float *out, float *in, int N, int SO, int SI)
{
	float	a = -1.586134342f,
		b = -0.05298011854f,
		c = 0.8829110762f,
		d = 0.4435068522f,
		e = 1.149604398f;

	for (int i = 0; i < N; i += 2) {
		out[(i+0)*SO] = in[(i+0)/2*SI] / e;
		out[(i+1)*SO] = in[(i+N)/2*SI] * e;
	}
	out[0] -= d * (2.f * out[SO]);
	for (int i = 2; i < N; i += 2)
		out[i*SO] -= d * (out[(i-1)*SO] + out[(i+1)*SO]);

	for (int i = 1; i < N-1; i += 2)
		out[i*SO] -= c * (out[(i-1)*SO] + out[(i+1)*SO]);
	out[(N-1)*SO] -= c * (2.f * out[(N-2)*SO]);

	out[0] -= b * (2.f * out[SO]);
	for (int i = 2; i < N; i += 2)
		out[i*SO] -= b * (out[(i-1)*SO] + out[(i+1)*SO]);

	for (int i = 1; i < N-1; i += 2)
		out[i*SO] -= a * (out[(i-1)*SO] + out[(i+1)*SO]);
	out[(N-1)*SO] -= a * (2.f * out[(N-2)*SO]);
}

