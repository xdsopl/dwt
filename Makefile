CFLAGS = -std=c99 -W -Wall -O3 -D_GNU_SOURCE=1 -g
LDLIBS = -lm

all: encode decode

test: encode decode
	./encode input.ppm putput.dwt
	./decode putput.dwt output.ppm

%: %.c *.h
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

clean:
	rm -f encode decode

