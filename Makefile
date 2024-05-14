CFLAGS = -std=c99 -W -Wall -Ofast
# CFLAGS += -g -fsanitize=address

all: encode decode

test: encode decode
	./encode input.ppm - | ./decode - output.ppm
	compare -verbose -metric PSNR input.ppm output.ppm /dev/null ; true

%: %.c *.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f encode decode

