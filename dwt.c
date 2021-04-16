/*
dwt - playing with dwt and lossy image compression
Written in 2014 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

float srgb(float v)
{
	float K0 = 0.04045f;
	float a = 0.055f;
	float phi = 12.92f;
	float gamma = 2.4f;
	return v <= K0 / phi ? v * phi : (1.0f + a) * powf(v, 1.0f / gamma) - a;
}

float linear(float v)
{
	float K0 = 0.04045f;
	float a = 0.055f;
	float phi = 12.92f;
	float gamma = 2.4f;
	return v <= K0 ? v / phi : powf((v + a) / (1.0f + a), gamma);
}

float fclampf(float x, float a, float b)
{
	return fminf(fmaxf(x, a), b);
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

void ycbcr_image(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		rgb2ycbcr(image->buffer + 3 * i);
}

void rgb_image(struct image *image)
{
	for (int i = 0; i < image->total; i++)
		ycbcr2rgb(image->buffer + 3 * i);
}

struct image *read_ppm(char *name)
{
	FILE *file = fopen(name, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to read.\n", name);
		return 0;
	}
	if ('P' != fgetc(file) || '6' != fgetc(file)) {
		fprintf(stderr, "file \"%s\" not P6 image.\n", name);
		fclose(file);
		return 0;
	}
	int integer[3];
	struct image *image = 0;
	int c = fgetc(file);
	if (EOF == c)
		goto eof;
	for (int i = 0; i < 3; i++) {
		while ('#' == (c = fgetc(file)))
			while ('\n' != (c = fgetc(file)))
				if (EOF == c)
					goto eof;
		while ((c < '0') || ('9' < c))
			if (EOF == (c = fgetc(file)))
				goto eof;
		char str[16];
		for (int n = 0; n < 16; n++) {
			if (('0' <= c) && (c <= '9') && n < 15) {
				str[n] = c;
				if (EOF == (c = fgetc(file)))
					goto eof;
			} else {
				str[n] = 0;
				break;
			}
		}
		integer[i] = atoi(str);
	}
	if (!(integer[0] && integer[1] && integer[2])) {
		fprintf(stderr, "could not read image file \"%s\".\n", name);
		fclose(file);
		return 0;
	}
	if (integer[2] != 255) {
		fprintf(stderr, "cant read \"%s\", only 8 bit per channel SRGB supported at the moment.\n", name);
		fclose(file);
		return 0;
	}
	image = new_image(name, integer[0], integer[1]);
	for (int i = 0; i < 3 * image->total; i++) {
		int v = fgetc(file);
		if (EOF == v)
			goto eof;
		image->buffer[i] = linear(v / 255.0f);
	}
	fclose(file);
	return image;
eof:
	fprintf(stderr, "EOF while reading from \"%s\".\n", name);
	fclose(file);
	delete_image(image);
	return 0;
}

int write_ppm(struct image *image)
{
	FILE *file = fopen(image->name, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file to write.\n", image->name);
		return 0;
	}
	if (!fprintf(file, "P6 %d %d 255\n", image->width, image->height)) {
		fprintf(stderr, "could not write to file \"%s\".\n", image->name);
		fclose(file);
		return 0;
	}
	for (int i = 0; i < 3 * image->total; i++) {
		if (EOF == fputc(255.0f * srgb(image->buffer[i]), file))
			goto eof;
	}
	fclose(file);
	return 1;
eof:
	fprintf(stderr, "EOF while writing to \"%s\".\n", image->name);
	fclose(file);
	return 0;
}

void haar(float *out, float *in, int N, int S)
{
	for (int l = N / 2; l > 0; l /= 2) {
		for (int i = 0; i < l; i++) {
			out[(0 + i) * S] = (in[(i * 2 + 0) * S] + in[(i * 2 + 1) * S]) / sqrtf(2.0f);
			out[(l + i) * S] = (in[(i * 2 + 0) * S] - in[(i * 2 + 1) * S]) / sqrtf(2.0f);
		}
		for (int i = 0; i < 2 * l; i++)
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

float quantization(int i, int j, int N, float min, float max)
{
	(void)N;
	return min + (max - min) * powf(2.0f, - (i + j));
//	return min + (max - min) * (2 * N - (i + j)) / (2.0f * N);
//	return min + (max - min) * (N * N - (i * j)) / (float)(N * N);
}

void blah(float *output, float *input, int N, float min, float max)
{
	for (int i = 0; i < N; i++)
		haar(output + 3 * N * i, input + 3 * N * i, N, 3);
	for (int i = 0; i < N * N; i++)
		input[i * 3] = output[i * 3];
	for (int i = 0; i < N; i++)
		haar(output + 3 * i, input + 3 * i, N, 3 * N);

#if 1
	for (int j = 0; j < N; j++) {
		for (int i = 0; i < N; i++) {
			float q = quantization(i, j, N, min, max);
			output[(N * j + i) * 3] = roundf(q * output[(N * j + i) * 3]) / q;
		}
	}
#endif
#if 1
	for (int i = 0; i < N * N; i++)
		input[i * 3] = output[i * 3];
	for (int i = 0; i < N; i++)
		ihaar(output + 3 * i, input + 3 * i, N, 3 * N);
	for (int i = 0; i < N * N; i++)
		input[i * 3] = output[i * 3];
	for (int i = 0; i < N; i++)
		ihaar(output + 3 * N * i, input + 3 * N * i, N, 3);
#endif
}

void doit(struct image *output, struct image *input)
{
	int N = output->width;
	ycbcr_image(input);
	blah(output->buffer + 0, input->buffer + 0, N, 64, 128);
	blah(output->buffer + 1, input->buffer + 1, N, 16, 32);
	blah(output->buffer + 2, input->buffer + 2, N, 16, 32);
	rgb_image(output);
}

int pow2(int N)
{
	return !(N & (N - 1));
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s input.ppm output.ppm\n", argv[0]);
		return 1;
	}
	struct image *input = read_ppm(argv[1]);
	if (!input || input->width != input->height || !pow2(input->width))
		return 1;
	struct image *output = new_image(argv[2], input->width, input->height);

	doit(output, input);

	if (!write_ppm(output))
		return 1;
	return 0;
}

