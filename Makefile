CCFLAGS=-Wall -Wpedantic -Wextra -lm -g

bin/rlplot: build/udp.o build/main.o build/graph.o build/refresh_shaders.o
	gcc $(CCFLAGS) -pthread -lraylib -o bin/rlplot build/udp.o build/main.o build/graph.o build/refresh_shaders.o
	cp /usr/lib/libraylib.so.420 bin/libraylib.so.420
	cp /usr/lib/libraylib.so.420 bin/libraylib.so.420
	test -d bin/shaders || mkdir bin/shaders && cp -r shaders/line.fs shaders/line.vs shaders/grid.fs bin/shaders
	test -f bin/rlplot.zip && rm bin/rlplot.zip || echo "ok"
	zip bin/rlplot.zip bin/* bin/**/*

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
