.PHONY: all
all: bin/brplot

bin/brplot: nob
	./nob

nob: nob.c external/nob.h
	cc -I. -o nob nob.c

.PHONY: install
install: nob
	./nob install
