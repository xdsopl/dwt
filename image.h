/*
Image buffer with color space conversion

Copyright 2014 Ahmet Inan <xdsopl@gmail.com>
*/

#pragma once

#include <stdlib.h>
#include <math.h>

struct image {
	float *buffer;
	int width, height, total;
	char *name;
};

void delete_image(struct image *image)
{
	free(image->buffer);
	free(image);
}

struct image *new_image(char *name, int width, int height)
{
	struct image *image = malloc(sizeof(struct image));
	image->height = height;
	image->width = width;
	image->total = width * height;
	image->name = name;
	image->buffer = malloc(3 * sizeof(float) * width * height);
	return image;
}

float fclampf(float x, float a, float b)
{
	return fminf(fmaxf(x, a), b);
}

float linear2srgb(float v)
{
	float K0 = 0.04045f;
	float a = 0.055f;
	float phi = 12.92f;
	float gamma = 2.4f;
	return v <= K0 / phi ? v * phi : (1.0f + a) * powf(v, 1.0f / gamma) - a;
}

float srgb2linear(float v)
{
	float K0 = 0.04045f;
	float a = 0.055f;
	float phi = 12.92f;
	float gamma = 2.4f;
	return v <= K0 ? v / phi : powf((v + a) / (1.0f + a), gamma);
}

void ycbcr2rgb(float *io)
{
	float WR = 0.2126f;
	float WB = 0.0722f;
	float WG = 1.0f - WR - WB;
	float UMAX = 0.5f;
	float VMAX = 0.5f;
	float y = io[0], u = io[1], v = io[2];
	io[0] = fclampf(y + (1.0f - WR) / VMAX * v, 0.0f, 1.0f);
	io[1] = fclampf(y - WB * (1.0f - WB) / (UMAX * WG) * u - WR * (1.0f - WR) / (VMAX * WG) * v, 0.0f, 1.0f);
	io[2] = fclampf(y + (1.0f - WB) / UMAX * u, 0.0f, 1.0f);
}

void rgb2ycbcr(float *io)
{
	float WR = 0.2126f;
	float WB = 0.0722f;
	float WG = 1.0f - WR - WB;
	float UMAX = 0.5f;
	float VMAX = 0.5f;
	float r = io[0], g = io[1], b = io[2];
	io[0] = fclampf(WR * r + WG * g + WB * b, 0.0f, 1.0f);
	io[1] = fclampf(UMAX / (1.0f - WB) * (b - io[0]), -UMAX, UMAX);
	io[2] = fclampf(VMAX / (1.0f - WR) * (r - io[0]), -VMAX, VMAX);
}

void rct2srgb(float *io)
{
	float Y = io[0];
	float U = io[1];
	float V = io[2];
	float G = Y - floorf((U + V) / 4.f);
	float R = U + G;
	float B = V + G;
	io[0] = fclampf(R, 0.f, 255.f);
	io[1] = fclampf(G, 0.f, 255.f);
	io[2] = fclampf(B, 0.f, 255.f);
}

void srgb2rct(float *io)
{
	float R = io[0];
	float G = io[1];
	float B = io[2];
	float Y = floorf((R + 2.f * G + B) / 4.f);
	float U = R - G;
	float V = B - G;
	io[0] = Y;
	io[1] = U;
	io[2] = V;
}

void ycocg2srgb(float *io)
{
	float Y = io[0];
	float U = io[1];
	float V = io[2];
	float T = Y - floorf(V / 2.f);
	float G = V + T;
	float B = T - floorf(U / 2.f);
	float R = B + U;
	io[0] = fclampf(R, 0.f, 255.f);
	io[1] = fclampf(G, 0.f, 255.f);
	io[2] = fclampf(B, 0.f, 255.f);
}

void srgb2ycocg(float *io)
{
	float R = io[0];
	float G = io[1];
	float B = io[2];
	float U = R - B;
	float T = B + floorf(U / 2.f);
	float V = G - T;
	float Y = T + floorf(V / 2.f);
	io[0] = Y;
	io[1] = U;
	io[2] = V;
}

void srgb_from_linear(struct image *image)
{
	for (int i = 0; i < 3 * image->total; i++)
		image->buffer[i] = 255.f * linear2srgb(fclampf(image->buffer[i], 0.f, 1.f));
}

void linear_from_srgb(struct image *image)
{
	for (int i = 0; i < 3 * image->total; i++)
		image->buffer[i] = srgb2linear(image->buffer[i] / 255.f);
}

void ycbcr_from_linear(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rgb2ycbcr(image->buffer + 3 * i);
}

void linear_from_ycbcr(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		ycbcr2rgb(image->buffer + 3 * i);
}

void ycbcr_from_srgb(struct image *image)
{
	for (int i = 0; i < image->total; i++) {
		for (int j = 0; j < 3; j++)
			image->buffer[3*i+j] = srgb2linear(image->buffer[3*i+j] / 255.f);
		rgb2ycbcr(image->buffer + 3 * i);
	}
}

void srgb_from_ycbcr(struct image *image)
{
	for (int i = 0; i < image->total; i++) {
		ycbcr2rgb(image->buffer + 3 * i);
		for (int j = 0; j < 3; j++)
			image->buffer[3*i+j] = 255.f * linear2srgb(fclampf(image->buffer[3*i+j], 0.f, 1.f));
	}
}

void rct_from_srgb(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		srgb2rct(image->buffer + 3 * i);
}

void srgb_from_rct(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rct2srgb(image->buffer + 3 * i);
}

void ycocg_from_srgb(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		srgb2ycocg(image->buffer + 3 * i);
}

void srgb_from_ycocg(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		ycocg2srgb(image->buffer + 3 * i);
}

