CC = gcc

all: vxsh.o

vxsh.o: main.c vxsh.c
	$(CC) main.c vxsh.c -o $@

clean: 
	-rm -rf vxsh.o

