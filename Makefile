CFLAGS = -std=c99 -W -Wall -O3 -D_GNU_SOURCE=1 -g
LDLIBS = -lm

all: encode decode

test: encode decode
	./encode input.ppm /dev/stdout | ./decode /dev/stdin output.ppm

clean:
	rm -f encode decode

