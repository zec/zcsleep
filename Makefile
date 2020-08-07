CC=gcc
COPTS=-D_POSIX_C_SOURCE=200112L -lm

all: zcsleep

.PHONY: all clean

zcsleep: zcsleep.c
	$(CC) $(COPTS) -o $@ $<

clean:
	rm -f zcsleep
