# This are the configuration options.
# default configuration (calling make with 0 arguments) call is:
# $ make CONFIG=RELEASE PLATFORM=LINUX TYPE=EXE COMPILER=GCC
# But you can change it by changing one param e.g. use clang:
# $ make COMPILER=CLANG

# DEBUG | RELEASE
CONFIG    ?= RELEASE
# LINUX | WEB | WINDOWS
PLATFORM  ?= LINUX
HEADLESS  ?= NO
# EXE | LIB
TYPE      ?= EXE
NATIVE_CC ?= cc

# Only flags for linux development
ifeq ($(PLATFORM), LINUX)
	# GCC | CLANG | MUSL | COSMO | ZIG
	COMPILER    ?= GCC
	# YES | NO
	COVERAGE    ?= NO
	# YES | NO
	FUZZ        ?= NO
	# YES | NO
	TRACY       ?= NO
	# YES | NO
	LTO         ?= YES
	# X11
	HAS_X11     ?= YES
	# Only when TYPE=LIB
	STATIC      ?= NO
	# Only in debug build
	SANITIZE    ?= YES
endif

SOURCE             = src/main.c           src/ui.c              src/data.c        src/mesh.c        src/q.c       src/read_input.c \
										 external/shl_impls.c src/resampling.c      src/shaders.c     src/plotter.c     src/anim.c             \
										 src/plot.c           src/permastate.c      src/filesystem.c  src/gui.c         src/text_renderer.c            \
										 src/data_generator.c src/platform2.c       src/threads.c     src/gl.c          src/icons.c   src/theme.c
COMMONFLAGS        = -I. -MMD -MP -fvisibility=hidden -std=gnu11
WARNING_FLAGS      = -Wconversion -Wall -Wextra -Wshadow -D_GNU_SOURCE -Wno-gnu-folding-constant
LD_FLAGS           =

ifeq ($(PLATFORM)_$(COMPILER), LINUX_CLANG)
	ifeq ($(COVERAGE), YES)
		COMMONFLAGS+= -fprofile-instr-generate -fcoverage-mapping -mllvm -runtime-counter-relocation
	endif
	ifeq ($(FUZZ), YES)
		COMMONFLAGS+= -fsanitize=fuzzer
	endif
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof -Wno-gnu-zero-variadic-macro-arguments
	CC= clang
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_GCC)
	CC= gcc
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_COSMO)
	CC= cosmocc
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_MUSL)
	CC= musl-gcc
	COMMONFLAGS+= -DBR_MUSL
	# MUSL fails with sanitizer...
	SANITIZE= NO
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_ZIG)
	CC= zig cc
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof -Wno-gnu-zero-variadic-macro-arguments
	# ZIG fails with sanitizer...
	SANITIZE= NO
endif

ifeq ($(HEADLESS), YES)
	COMMONFLAGS+= -DNUMBER_OF_STEPS=100 -DHEADLESS
endif

ifeq ($(PLATFORM), LINUX)
	SHADERS_HEADER= .generated/shaders.h
	ifeq ($(TYPE), LIB)
		COMMONFLAGS+= -fPIC -DBR_LIB
		LD_FLAGS+= -fPIC -shared
	endif
	ifeq ($(COMPILER), MUSL)
		LIBS+= -l:libm.a -l:libdl.a -l:libc.a
	else
		LIBS+= -lm -ldl -pthread
	endif
	ifeq ($(HAS_X11), NO)
		COMMONFLAGS+= -DBR_NO_X11
	endif
	ifeq ($(HEADLESS)_$(HAS_X11), YES_YES)
		BACKENDS= HX
	else ifeq ($(HEADLESS)_$(HAS_X11), YES_NO)
		BACKENDS= H
	else ifeq ($(HEADLESS)_$(HAS_X11), NO_YES)
		BACKENDS= X
	else
		$(error This is not valid configuration)
	endif

else ifeq ($(PLATFORM), WINDOWS)
	CC= x86_64-w64-mingw32-gcc
	COMMONFLAGS+= -D_WIN32=1 -DWIN32_LEAN_AND_MEAN
	LD_FLAGS+= -static
	SHADERS_HEADER= .generated/shaders.h
	COMPILER= MINGW
	BACKENDS= WINDOWS

else ifeq ($(PLATFORM), WEB)
	CC= $(EMSCRIPTEN)emcc
	COMMONFLAGS+= -DGRAPHICS_API_OPENGL_ES3=1
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof -Wno-gnu-zero-variadic-macro-arguments
	LD_FLAGS= -sWASM_BIGINT -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sGL_ENABLE_GET_PROC_ADDRESS
	LD_FLAGS+= -sCHECK_NULL_WRITES=0 -sDISABLE_EXCEPTION_THROWING=1 -sFILESYSTEM=0 -sDYNAMIC_EXECUTION=0
	SHADERS_HEADER= .generated/shaders_web.h
	COMPILER= EMCC
	NOBS+= www/.nob.dir
	ifeq ($(TYPE), LIB)
		COMMONFLAGS+= -DBR_LIB
		LD_FLAGS+= -sMODULARIZE=1 -sEXPORT_ES6=1
		OUTPUT= $(shell echo 'www/brplot_$(HEADLESS)_$(CONFIG)_lib.js' | tr '[A-Z]' '[a-z]')
	else ifeq ($(TYPE), EXE)
		OUTPUT= $(shell echo 'www/brplot_$(HEADLESS)_$(CONFIG).html' | tr '[A-Z]' '[a-z]')
		LD_FLAGS+= -sASYNCIFY
	else
		echo "Valid TYPE parameter values are LIB, EXE" && exit -1
	endif
else
	echo "Valid PLATFORM parameter values are LINUX, WINDOWS, WEB" && exit -1
endif

ifeq ($(CONFIG), DEBUG)
	COMMONFLAGS+= -ggdb -DBR_DEBUG
	SHADERS_HEADER=
	ifeq ($(PLATFORM), LINUX)
		ifeq ($(SANITIZE), YES)
			COMMONFLAGS+= -DBR_ASAN
			ifeq ($(COMPILER), GCC)
				COMMONFLAGS+= -fsanitize=bounds-strict
			endif
			ifeq ($(COVERAGE), NO)
				LD_FLAGS+= -rdynamic
				ifeq ($(TYPE), EXE)
					COMMONFLAGS+= -fpie \
					 -fsanitize=address -fsanitize=leak \
					 -fsanitize=undefined -fsanitize=signed-integer-overflow \
					 -fsanitize=integer-divide-by-zero -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
				endif
				COMMONFLAGS+= -pg
			endif
		endif
	endif
else ifeq ($(CONFIG), RELEASE)
	COMMONFLAGS+= -fdata-sections -ffunction-sections -Os -ggdb
	LD_FLAGS+= -fdata-sections -ffunction-sections -Wl,--gc-sections
	ifeq ($(PATFORM)_$(LTO), LINUX_YES)
		LD_FLAGS+= -flto=auto
	endif
else
	$(error BadCONFIG)
endif

ifeq ($(TRACY), YES)
	COMMONFLAGS+= -DTRACY_ENABLE=1
	LD_FLAGS+=
	LIBS+= /usr/lib/libTracyClient.a
endif

CCFLAGS= $(COMMONFLAGS) -std=gnu11
ifeq ($(TYPE), EXE)
	PREFIX_BUILD= $(shell echo 'build/$(PLATFORM)/$(CONFIG)/$(BACKENDS)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
	OUTPUT?= $(shell echo 'bin/brplot_$(PLATFORM)_$(CONFIG)_$(BACKENDS)_$(COMPILER)' | tr '[A-Z]' '[a-z]')
else
	ifeq ($(STATIC), YES)
		PREFIX_BUILD= $(shell echo 'build/slib/$(PLATFORM)/$(CONFIG)/$(BACKENDS)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
		OUTPUT?= $(shell echo   'bin/libbrplot_$(PLATFORM)_$(CONFIG)_$(BACKENDS)_$(COMPILER).a' | tr '[A-Z]' '[a-z]')
	else
		PREFIX_BUILD= $(shell echo 'build/dlib/$(PLATFORM)/$(CONFIG)/$(BACKENDS)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
		OUTPUT?= $(shell echo   'bin/libbrplot_$(PLATFORM)_$(CONFIG)_$(BACKENDS)_$(COMPILER).so' | tr '[A-Z]' '[a-z]')
	endif
endif

OBJS= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(SOURCE))
MAKE_INCLUDES= $(patsubst %.o, %.d, $(OBJS))
NOBS+= $(patsubst %.o, %.nob.dir, $(OBJS))
NOBS+= bin/.nob.dir .generated/.nob.dir


LD= $(CC)

$(OUTPUT):

ifeq ($(STATIC), YES)
%.a: $(OBJS)
	ar rcs $@ $^
else
$(OUTPUT): $(OBJS)
	$(LD) $(COMMONFLAGS) $(LD_FLAGS) -o $@ $(OBJS) $(LIBS)
ifeq ($(TYPE), EXE)
	ln -fs $@ brplot
else
	ln -fs $@ libbrplot.so
endif
endif

$(PREFIX_BUILD)/src/%.o: src/%.c
	$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o: %.c
	$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(OBJS): $(NOBS)

src/*.c: .generated/icons.h .generated/icons.c .generated/gl.c .generated/icons.h .generated/icons.c .generated/default_font.h
src/shaders.c: $(SHADERS_HEADER)

.PHONY: clean
clean:
	test -d .generated && rm .generated -rdf || echo "done"
	test -d build &&  rm build -rdf || echo "done"
	test -d bin &&  rm bin -rdf || echo "done"
	test -d www &&  rm www -rdf || echo "done"
	test -d zig-cache && rm zig-cache -rdf || echo "done"
	test -d .zig-cache && rm .zig-cache -rdf || echo "done"
	test -d zig-out && rm zig-out -rdf || echo "done"
	test -f src/misc/shaders.h && rm src/misc/shaders.h || echo "done"
	test -f src/misc/shaders_web.h && rm src/misc/shaders_web.h || echo "done"
	test -f src/misc/default_font.h && rm src/misc/default_font.h || echo "done"

.generated/default_font.h: bin/font_bake content/font.ttf
	bin/font_bake  content/font.ttf > .generated/default_font.h

bin/font_bake: tools/font_bake.c $(NOBS)
	$(NATIVE_CC) -O3 -o bin/font_bake tools/font_bake.c

bin/shaders_bake: tools/shaders_bake.c src/br_shaders.h $(NOBS)
	$(NATIVE_CC) -I. -O3 -o bin/shaders_bake tools/shaders_bake.c src/filesystem.c

$(SHADERS_HEADER): ./src/shaders/* bin/shaders_bake
	bin/shaders_bake $(PLATFORM) > $(SHADERS_HEADER)

.generated/icons.h .generated/icons.c: bin/pack_icons content/*.png
	bin/pack_icons > /dev/null

bin/pack_icons: tools/pack_icons.c $(NOBS)
	$(NATIVE_CC) -I. -O0 -o bin/pack_icons tools/pack_icons.c -lm

.generated/gl.c: bin/gl_gen
	bin/gl_gen 

bin/gl_gen: tools/gl_gen.c $(NOBS)
	$(NATIVE_CC) -I. -O0 -ggdb -g3 -o bin/gl_gen tools/gl_gen.c

.generated/brplot.c: bin/cshl
	./bin/cshl
-include .generated/brplot.c.d

bin/cshl: tools/create_single_header_lib.c
	$(NATIVE_CC) -I. -O3 -o bin/cshl  tools/create_single_header_lib.c


.PHONY: bench
bench: bin/bench
	date >> bench.txt
	./bin/bench >> bench.txt
	cat bench.txt

SOURCE_BENCH= ./src/misc/benchmark.c ./src/resampling.c ./src/mesh.c ./src/shaders.c ./src/plotter.c ./src/gui.c ./src/data.c ./src/plot.c ./src/q.c ./src/data_generator.c ./src/platform.c  ./src/permastate.c ./src/filesystem.c
OBJS_BENCH= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(SOURCE_BENCH))

bin/bench: $(OBJS_BENCH) $(NOBS)
	$(CC) $(LD_FLAGS) -o bin/bench $(COMMONFLAGS) $(LIBS) $(OBJS_BENCH)

-include $(MAKE_INCLUDES)

%nob.dir:
	@mkdir -p $(dir $@)
	@touch $@
