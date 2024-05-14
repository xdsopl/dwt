CFLAGS = -std=c99 -W -Wall -Ofast
# CFLAGS += -g -fsanitize=address

all: encode decode

test: encode decode
	./encode input.pnm - | ./decode - output.pnm
	compare -verbose -metric PSNR input.pnm output.pnm /dev/null ; true

%: %.c *.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f encode decode

