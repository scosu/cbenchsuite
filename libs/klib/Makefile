.PHONY: all
all: libklib.a klib.o

CC := gcc
CFLAGS := -Wall -Iinclude -O2

src_files = $(wildcard *.c)

libklib.a: klib.o
	ar rcs $@ $^

klib.o: $(src_files)
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f klib.o libklib.a
