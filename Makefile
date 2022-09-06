CFLAGS = -std=c99 -W -Wall -O3 -D_GNU_SOURCE=1 -g -fsanitize=address
LDLIBS = -lm
RM = rm -f
COMPARE = compare -verbose -metric PSNR

all: dwtenc dwtdec

test: dwtenc dwtdec
	./dwtenc input.ppm /dev/stdout | ./dwtdec /dev/stdin output.ppm
	$(COMPARE) input.ppm output.ppm /dev/null ; true

dwtenc: src/encode.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

dwtdec: src/decode.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

clean:
	$(RM) dwtenc dwtdec

