/*
Discrete wavelet transform

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

void dwt2d(void (*wavelet)(float *, float *, int, int, int, int), float *out, float *in, int N0, int W, int H, int SO, int SI, int SW, int CH)
{
	for (int j = 0; j < H; ++j) {
		wavelet(out+SO*SW*j, in+SI*SW*j, W, SO*CH, SI*CH, CH);
		for (int i = 0; i < W*CH; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	}
	wavelet(out, in, H, SO*SW, SI*SW, W*CH);
	int W2 = (W+1)/2, H2 = (H+1)/2;
	for (int j = 0; j < H2; ++j)
		for (int i = 0; i < W2*CH; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	if (W2 >= N0 && H2 >= N0)
		dwt2d(wavelet, out, in, N0, W2, H2, SO, SI, SW, CH);
}

void idwt2d(void (*iwavelet)(float *, float *, int, int, int, int), float *out, float *in, int N0, int W, int H, int SO, int SI, int SW, int CH)
{
	int W2 = (W+1)/2, H2 = (H+1)/2;
	if (W2 >= N0 && H2 >= N0)
		idwt2d(iwavelet, out, in, N0, W2, H2, SO, SI, SW, CH);
	iwavelet(out, in, H, SO*SW, SI*SW, W*CH);
	for (int j = 0; j < H; ++j)
		for (int i = 0; i < W*CH; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	for (int j = 0; j < H; ++j) {
		iwavelet(out+SO*SW*j, in+SI*SW*j, W, SO*CH, SI*CH, CH);
		for (int i = 0; i < W*CH; ++i)
			in[(SW*j+i)*SI] = out[(SW*j+i)*SO];
	}
}

