CC=gcc
COPTS=

all: zcsleep

.PHONY: all clean

zcsleep: zcsleep.c
	$(CC) $(COPTS) -o $@ $<

clean:
	rm -f zcsleep
