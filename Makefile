# This are the configuration options.
# default configuration (calling make with 0 arguments) call is:
# $ make CONFIG=RELEASE PLATFORM=LINUX GUI=IMGUI TYPE=EXE COMPILER=GCC
# But you can change it by changing one param e.g. use clang:
# $ make COMPILER=CLANG

# DEBUG | RELEASE
CONFIG    ?= RELEASE
# LINUX | WEB | WINDOWS
PLATFORM  ?= LINUX
# IMGUI | RAYLIB | HEADLESS
GUI       ?= IMGUI
# EXE | LIB
TYPE      ?= EXE
# GCC | CLANG ( Only for linux build )
COMPILER  ?= GCC
NATIVE_CC ?= gcc
# YES | NO
COVERAGE  ?= NO
# YES | NO
FUZZ      ?= NO
# YES | NO
TRACY     ?= NO

RL                 = ./external/raylib-5.0/src
IM                 = ./external/imgui-docking
RAYLIB_SOURCES     = $(RL)/rmodels.c $(RL)/rshapes.c $(RL)/rtext.c $(RL)/rtextures.c $(RL)/utils.c $(RL)/rcore.c
SOURCE             = src/main.c src/help.c src/data.c src/smol_mesh.c src/q.c src/read_input.c src/plotter.c \
										 src/keybindings.c src/str.c src/memory.cpp src/resampling2.c src/graph_utils.c src/shaders.c \
										 src/plot.c src/permastate.c src/filesystem.c src/filesystem++.cpp src/gui.c src/gui++.cpp \
										 src/data_generator.c
COMMONFLAGS        = -I. -I./external/glfw/include/ -I./external/Tracy -I$(RL) -MMD -MP 
WARNING_FLAGS      = -Wconversion -Wall -Wpedantic -Wextra
LD_FLAGS           =

ifeq ($(PLATFORM)_$(COMPILER), LINUX_CLANG)
	ifeq ($(COVERAGE), YES)
		COMMONFLAGS+= -fprofile-instr-generate -fcoverage-mapping -mllvm -runtime-counter-relocation
	endif
	ifeq ($(FUZZ), YES)
		COMMONFLAGS+= -fsanitize=fuzzer
	endif
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof
	CXX= clang++
	CC= clang
else ifeq ($(PLATFORM)_$(COMPILER), LINUX_GCC)
	CXX= g++
	CC= gcc
endif

ifeq ($(GUI), IMGUI)
	SOURCE+= $(IM)/imgui.cpp $(IM)/imgui_draw.cpp $(IM)/imgui_tables.cpp \
				  $(IM)/imgui_widgets.cpp $(IM)/backends/imgui_impl_glfw.cpp $(IM)/backends/imgui_impl_opengl3.cpp \
					$(RAYLIB_SOURCES)
	COMMONFLAGS+= -I$(IM) -DIMGUI

else ifeq ($(GUI), RAYLIB)
	SOURCE+= $(RAYLIB_SOURCES)
	COMMONFLAGS+= -DRAYLIB

else ifeq ($(GUI), HEADLESS)
	COMMONFLAGS+= -DNUMBER_OF_STEPS=100 -DHEADLESS

else
	echo "Valid GUI parameters are IMGUI, RAYLIB, HEADLESS" && exit -1
endif

ifeq ($(PLATFORM), LINUX)
	LIBS= `pkg-config --static --libs glfw3` -lGL
	COMMONFLAGS+= -DLINUX=1 -DPLATFORM_DESKTOP=1
	SHADERS_HEADER= src/misc/shaders.h

else ifeq ($(PLATFORM), WINDOWS)
	LIBS= -lopengl32 -lgdi32 -lwinmm
	CXX= x86_64-w64-mingw32-g++
	CC= x86_64-w64-mingw32-gcc
	COMMONFLAGS+= -Iexternal/glfw/include -DWINDOWS=1 -DPLATFORM_DESKTOP=1 -D_WIN32=1 -DWIN32_LEAN_AND_MEAN
	SOURCE+= $(RL)/rglfw.c
	SHADERS_HEADER= src/misc/shaders.h
	COMPILER= MINGW

else ifeq ($(PLATFORM), WEB)
	CXX= $(EMSCRIPTEN)em++
	CC= $(EMSCRIPTEN)emcc
	COMMONFLAGS+= -DGRAPHICS_API_OPENGL_ES3=1 -DPLATFORM_WEB=1
	WARNING_FLAGS+= -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof
	LD_FLAGS= -sWASM_BIGINT -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sGL_ENABLE_GET_PROC_ADDRESS --shell-file=src/web/minshell.html
	LD_FLAGS+= -sCHECK_NULL_WRITES=0 -sDISABLE_EXCEPTION_THROWING=1 -sFILESYSTEM=0 -sDYNAMIC_EXECUTION=0
	SHADERS_HEADER= src/misc/shaders_web.h
	COMPILER= EMCC
	ifeq ($(TYPE), LIB)
		COMMONFLAGS+= -DLIB
		LD_FLAGS+= -sMODULARIZE=1 -sEXPORT_ES6=1
		OUTPUT= $(shell echo 'www/brplot_$(GUI)_$(CONFIG)_lib.js' | tr '[A-Z]' '[a-z]')
	else ifeq ($(TYPE), EXE)
		OUTPUT= $(shell echo 'www/brplot_$(GUI)_$(CONFIG).html' | tr '[A-Z]' '[a-z]')
		LD_FLAGS+= -sASYNCIFY
	else
		echo "Valid TYPE parameter values are LIB, EXE" && exit -1
	endif
else
	echo "Valid PLATFORM parameter values are LINUX, WINDOWS, WEB" && exit -1
endif

ifeq ($(CONFIG), DEBUG)
	COMMONFLAGS+= -g
	SHADERS_HEADER=
	ifeq ($(PLATFORM), LINUX)
		COMMONFLAGS+= -DUNIT_TEST
		ifeq ($(COMPILER), GCC)
		  COMMONFLAGS+= -fsanitize=bounds-strict
		endif
		ifeq ($(COVERAGE), NO)
			LD_FLAGS+= -rdynamic
			COMMONFLAGS+= -fpie -pg \
			 -fsanitize=address -fsanitize=leak \
			 -fsanitize=undefined -fsanitize=signed-integer-overflow \
			 -fsanitize=integer-divide-by-zero -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
		endif
	endif
else ifeq ($(CONFIG), RELEASE)
	COMMONFLAGS+= -fdata-sections -ffunction-sections -Os -DRELEASE=1 \
		-DIMGUI_DISABLE_DEMO_WINDOWS \
		-DIMGUI_DISABLE_DEBUG_TOOLS
	LD_FLAGS+= -fdata-sections -ffunction-sections -Wl,--gc-sections 
	ifeq ($(PLATFORM), LINUX)
		LD_FLAGS+= -flto=auto
	endif
else
	$(error BadCONFIG)
endif

ifeq ($(TRACY), YES)
	COMMONFLAGS+= -DTRACY_ENABLE=1
	LD_FLAGS+= -ltracy
endif

PREFIX_BUILD= $(shell echo 'build/$(PLATFORM)/$(CONFIG)/$(GUI)/$(COMPILER)' | tr '[A-Z]' '[a-z]')
OBJSA= $(patsubst %.cpp, $(PREFIX_BUILD)/%.o, $(SOURCE))
OBJS+= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(OBJSA))
MAKE_INCLUDES= $(patsubst %.o, %.d, $(OBJS))
CXXFLAGS= $(COMMONFLAGS) -fno-exceptions -std=gnu++17
CCFLAGS= $(COMMONFLAGS)
OUTPUT?= $(shell echo 'bin/brplot_$(GUI)_$(PLATFORM)_$(CONFIG)_$(COMPILER)' | tr '[A-Z]' '[a-z]')

OBJSDIR= $(sort $(dir $(OBJS)))
$(shell $(foreach var,$(OBJSDIR), test -d $(var) || mkdir -p $(var);))
$(shell test -d $(dir $(OUTPUT)) || mkdir $(dir $(OUTPUT)))
$(shell test -d bin || mkdir bin)

$(OUTPUT): $(OBJS)
	$(CXX) $(COMMONFLAGS) $(LD_FLAGS) -o $@ $(LIBS) $(OBJS) $(LIBS)
	ln -fs $@ brplot

$(PREFIX_BUILD)/src/%.o:src/%.c
	$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(PREFIX_BUILD)/src/%.o:src/%.cpp
	$(CXX) $(CXXFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o:%.c
	$(CC) $(CCFLAGS) -c -o $@ $<

src/help.c: src/misc/default_font.h
ifeq ($(CONFIG), DEBUG)
endif
src/shaders.c: $(SHADERS_HEADER)

.PHONY: clean
clean:
	test -d build &&  rm build -rdf || echo "done"
	test -d bin &&  rm bin -rdf || echo "done"
	test -d www &&  rm www -rdf || echo "done"
	test -d zig-cache && rm zig-cache -rdf || echo "done"
	test -d zig-out && rm zig-out -rdf || echo "done"
	test -f src/misc/shaders.h && rm src/misc/shaders.h || echo "done"
	test -f src/misc/shaders_web.h && rm src/misc/shaders_web.h || echo "done"
	test -f src/misc/default_font.h && rm src/misc/default_font.h || echo "done"

.PHONY: fuzz
fuzz:
	make CONFIG=DEBUG GUI=HEADLESS && \
	cat /dev/random | ./bin/brplot_headless_linux_debug_gcc > /dev/null && echo "Fuzz test OK"

.PHONY: test
test:
	make GUI=HEADLESS CONFIG=DEBUG && \
	./bin/brplot_headless_linux_debug_gcc --unittest

.PHONY: test-gdb
test-gdb:
	make GUI=HEADLESS CONFIG=DEBUG && \
	gdb -ex "r --unittest" ./bin/brplot_headless_linux_debug_gcc --tui

.PHONY: npm-imgui
npm-imgui:
	make GUI=IMGUI CONFIG=RELEASE TYPE=LIB PLATFORM=WEB && \
	cp ./www/brplot_imgui_release_lib.js packages/npm/brplot.js && \
	cp ./www/brplot_imgui_release_lib.wasm packages/npm && \
	  ((cd packages/npm && \
	   npm publish || cd ../..) && cd ../..)

src/misc/default_font.h: bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf
	bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf > src/misc/default_font.h

bin/font_export: tools/font_export.c
	$(NATIVE_CC) -o bin/font_export tools/font_export.c -lm

bin/shaders_bake: ./tools/shaders_bake.c ./src/br_shaders.h ./src/str.c
	$(NATIVE_CC) -I. -I./external/raylib-5.0/src -O3 -o bin/shaders_bake src/str.c tools/shaders_bake.c

$(SHADERS_HEADER): ./src/desktop/shaders/* bin/shaders_bake
	bin/shaders_bake $(PLATFORM) > $(SHADERS_HEADER)

COMPILE_FLAGS_JSONA= $(patsubst %.cpp, $(PREFIX_BUILD)/%.json, $(SOURCE))
COMPILE_FLAGS_JSON= $(patsubst %.c, $(PREFIX_BUILD)/%.json, $(COMPILE_FLAGS_JSONA))

PWD= $(shell pwd)

compile_commands.json: $(COMPILE_FLAGS_JSON)
	echo "[" > compile_commands.json
	cat $(COMPILE_FLAGS_JSON) >> compile_commands.json
	echo "]" >> compile_commands.json

$(PREFIX_BUILD)/src/%.json:src/%.c
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

$(PREFIX_BUILD)/src/%.json:src/%.cpp
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CXX) $(CXXFLAGS) $(WARNING_FLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

$(PREFIX_BUILD)/%.json:%.c
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CC) $(CCFLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

$(PREFIX_BUILD)/%.json:%.cpp
	echo '{' > $@ && \
  echo '"directory": "$(PWD)",' >> $@ && \
  echo '"command": "$(CXX) $(CXXFLAGS) -c $<",' >> $@ && \
  echo '"file": "$<"' >> $@ && \
	echo '},' >> $@

-include $(MAKE_INCLUDES)
