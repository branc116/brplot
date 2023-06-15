CCFLAGS=-Wall -Wpedantic -Wextra -lm -O3

bin/rlplot: build/udp.o build/main.o build/graph.o build/refresh_shaders.o
	gcc $(CCFLAGS) -pthread -lraylib -o bin/rlplot build/udp.o build/main.o build/graph.o build/refresh_shaders.o

build/udp.o: udp.c udp.h
	gcc $(CCFLAGS) -c -o build/udp.o udp.c

build/main.o: main.c udp.h graph.h refresh_shaders.h
	gcc $(CCFLAGS) -c -o build/main.o main.c

build/graph.o: graph.c graph.h
	gcc $(CCFLAGS) -c -o build/graph.o graph.c

build/refresh_shaders.o: refresh_shaders.c refresh_shaders.h
	gcc $(CCFLAGS) -c -o build/refresh_shaders.o refresh_shaders.c

PHONY: clean
clean:
	rm build/*
	rm bin/*
