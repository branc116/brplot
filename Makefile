CCFLAGS=-Wall -Wpedantic -Wextra -g

bin/rlplot: build/udp.o build/main.o
	gcc -pthread -lraylib -o bin/rlplot build/udp.o build/main.o

build/udp.o: udp.c udp.h
	gcc $(CCFLAGS) -c -o build/udp.o udp.c

build/main.o: main.c udp.h
	gcc $(CCFLAGS) -c -o build/main.o main.c

