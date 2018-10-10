CC = gcc

all: vxsh.o

vxsh.o: src/main.c src/vxsh.c
	$(CC) src/main.c src/vxsh.c -o $@

clean: 
	-rm -rf vxsh.o

