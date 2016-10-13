all : wadder baked_texture_data.o baked_map_data.o

CFLAGS?=-m32
CC?=gcc

wadder : wadder.c
	gcc -o $@ $^

rawwad.o : wadder doom1.wad
	./wadder
	gcc -o rawwad.o rawwad.c -c -m32

baked_texture_data.o : baked_texture_data.c
	$(CC) -o $@ $^ -c $(CFLAGS)

baked_map_data.o : baked_map_data.c
	$(CC) -o $@ $^ -c  $(CFLAGS)

clean :
	rm -rf wadder baked_map_data.o baked_texture_data.o
