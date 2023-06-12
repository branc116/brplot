CCFLAGS=-Wall -Wpedantic -Wextra -g

bin/rlplot: build/udp.o build/main.o build/graph.o
	gcc -pthread -lraylib -o bin/rlplot build/udp.o build/main.o build/graph.o

build/udp.o: udp.c udp.h
	gcc $(CCFLAGS) -c -o build/udp.o udp.c

build/main.o: main.c udp.h graph.h
	gcc $(CCFLAGS) -c -o build/main.o main.c

build/graph.o: graph.c graph.h
	gcc $(CCFLAGS) -c -o build/graph.o graph.c


