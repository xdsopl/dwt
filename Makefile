CFLAGS = -std=c99 -W -Wall -O3 -D_GNU_SOURCE=1 -g
LDFLAGS = -lm

all: dwt

test: dwt
	./dwt input.ppm output.ppm

dwt: dwt.c

clean:
	rm -f dwt

