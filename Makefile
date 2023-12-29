RL= raylib/src
# DEBUG | RELEASE
CONFIG?= DEBUG
# LINUX | WEB | WINDOWS
PLATFORM?= LINUX
# IMGUI | RAYLIB | HEADLESS
GUI?= IMGUI
# EXE | LIB
TYPE?= EXE

RAYLIB_SOURCES     = $(RL)/rmodels.c $(RL)/rshapes.c $(RL)/rtext.c $(RL)/rtextures.c $(RL)/utils.c $(RL)/rcore.c
SOURCE             = src/main.c src/help.c src/points_group.c src/resampling.c src/smol_mesh.c src/q.c src/read_input.c src/gui.c src/keybindings.c src/str.c src/memory.cpp
EXTERNAL_HEADERS   =
ADDITIONAL_HEADERS = src/misc/default_font.h
RAYLIB_HEADERS     =  raylib/src/rcamera.h raylib/src/raymath.h raylib/src/raylib.h raylib/src/utils.h raylib/src/rlgl.h src/raylib/config.h
BR_HEADERS         = src/br_plot.h src/br_gui_internal.h src/br_help.h
COMMONFLAGS        = -I./imgui -I./imgui/backends -I. -Isrc/raylib -Iraylib/src
WARNING_FLAGS      = -Wconversion -Wall -Wpedantic -Wextra
LD_FLAGS=


# To debug include files use -H flag

ifeq ($(GUI), IMGUI)
	SOURCE+= imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp \
				  imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp \
				  src/imgui/gui.cpp src/imgui/ui_settings.cpp src/imgui/ui_info.cpp src/imgui/imgui_extensions.cpp $(RAYLIB_SOURCES)
	COMMONFLAGS+= -DIMGUI
	BR_HEADERS+= src/imgui/imgui_extensions.h
	ADDITIONAL_HEADERS+= $(RAYLIB_HEADERS)

else ifeq ($(GUI), RAYLIB)
	SOURCE+= src/raylib/gui.c src/raylib/ui.c $(RAYLIB_SOURCES)
	ADDITIONAL_HEADERS+= $(RAYLIB_HEADERS)

else ifeq ($(GUI), HEADLESS)
	SOURCE+= src/headless/raylib_headless.c src/headless/gui.c
	PLATFORM= LINUX
	COMMONFLAGS+= -DNUMBER_OF_STEPS=100

else
	echo "Valid GUI parameters are IMGUI, RAYLIB, HEADLESS" && exit -1
endif
	
ifeq ($(PLATFORM), LINUX)
	LIBS= `pkg-config --static --libs glfw3` -lGL
	CXX= g++
	CC= gcc
	COMMONFLAGS+= -DLINUX=1 -DPLATFORM_DESKTOP=1
	SOURCE+= src/desktop/linux/read_input.c src/desktop/platform.c
	SHADERS_HEADER= src/misc/shaders.h
	SHADERS_LIST= src/desktop/shaders/grid.fs src/desktop/shaders/line.fs src/desktop/shaders/line.vs src/desktop/shaders/quad.vs src/desktop/shaders/quad.fs

else ifeq ($(PLATFORM), WINDOWS)
	LIBS= -lopengl32 -lgdi32 -lwinmm
	CXX= x86_64-w64-mingw32-g++
	CC= x86_64-w64-mingw32-gcc
	COMMONFLAGS+= -Iraylib/src/external/glfw/include -DWINDOWS=1 -DPLATFORM_DESKTOP=1 -D_WIN32=1
	SOURCE+= $(RL)/rglfw.c src/desktop/win/read_input.c src/desktop/platform.c
	SHADERS_HEADER= src/misc/shaders.h
	SHADERS_LIST= src/desktop/shaders/grid.fs src/desktop/shaders/line.fs src/desktop/shaders/line.vs src/desktop/shaders/quad.vs src/desktop/shaders/quad.fs

else ifeq ($(PLATFORM), WEB)
	CXX= $(EMSCRIPTEN)em++
	CC= $(EMSCRIPTEN)emcc
	COMMONFLAGS+= -DGRAPHICS_API_OPENGL_ES2=1 -DPLATFORM_WEB=1
	LD_FLAGS= -sWASM_BIGINT -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sASYNCIFY -sGL_ENABLE_GET_PROC_ADDRESS --shell-file=src/web/minshell.html
	SOURCE+= src/web/read_input.c src/web/glfw_mock.c src/web/public_api.c
	SHADERS_LIST= src/web/shaders/grid.fs src/web/shaders/line.fs src/web/shaders/line.vs src/web/shaders/quad.fs src/web/shaders/quad.vs
	SHADERS_HEADER= src/misc/shaders_web.h
	OUTPUT= $(shell echo 'www/brplot_$(GUI)_$(CONFIG).html' | tr '[A-Z]' '[a-z]')
	ifeq ($(TYPE), LIB)
		COMMONFLAGS+= -DLIB
		LD_FLAGS+= -sMODULARIZE=1 -sEXPORT_ES6=1
		OUTPUT= $(shell echo 'www/brplot_$(GUI)_$(CONFIG)_lib.js' | tr '[A-Z]' '[a-z]')
	endif
else
	echo "Valid PLATFORM parameter values are LINUX, WINDOWS, WEB" && exit -1
endif

ifeq ($(CONFIG), DEBUG)
	COMMONFLAGS+= -g
	ifeq ($(PLATFORM), LINUX)
		SOURCE+= src/desktop/linux/refresh_shaders.c
		COMMONFLAGS+= -rdynamic -fpie -pg -DUNIT_TEST \
	   -fsanitize=address -fsanitize=leak \
		 -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=signed-integer-overflow \
		 -fsanitize=integer-divide-by-zero -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
	endif
	ifeq ($(PLATFORM), WINDOWS)
		SOURCE+= src/desktop/win/refresh_shaders.c
	endif
	ifeq ($(PLATFORM), WEB)
		SOURCE+= src/web/refresh_shaders.c
	endif
	ifeq ($(GUI), IMGUI)
		SOURCE+= imgui/imgui_demo.cpp
		ifeq ($(PLATFORM), LINUX)
			SOURCE+= src/imgui/hotreload.c
		endif
	endif
else ifeq ($(CONFIG), RELEASE)
	COMMONFLAGS+= -Os -DRELEASE=1 \
		-DIMGUI_DISABLE_DEMO_WINDOWS \
		-DIMGUI_DISABLE_DEBUG_TOOLS
	ADDITIONAL_HEADERS+= $(SHADERS_HEADER)
	ifeq ($(PLATFORM), LINUX)
		LD_FLAGS+= -flto=auto
	endif
else
	$(error BadCONFIG)
endif


PREFIX_BUILD= $(shell echo 'build/$(PLATFORM)/$(CONFIG)/$(GUI)' | tr '[A-Z]' '[a-z]')
OBJSA= $(patsubst %.cpp, $(PREFIX_BUILD)/%.o, $(SOURCE))
OBJS+= $(patsubst %.c, $(PREFIX_BUILD)/%.o, $(OBJSA))
CXXFLAGS= $(COMMONFLAGS)
CCFLAGS= $(COMMONFLAGS)
OUTPUT?= $(shell echo 'bin/brplot_$(GUI)_$(PLATFORM)_$(CONFIG)' | tr '[A-Z]' '[a-z]')

OBJSDIR= $(sort $(dir $(OBJS)))
$(shell $(foreach var,$(OBJSDIR), test -d $(var) || mkdir -p $(var);))
$(shell test -d $(dir $(OUTPUT)) || mkdir $(dir $(OUTPUT)))
$(shell test -d bin || mkdir bin)

$(OUTPUT): $(ADDITIONAL_HEADERS) $(OBJS)
	$(CXX) $(COMMONFLAGS) $(LD_FLAGS) -o $@ $(LIBS) $(OBJS) $(LIBS)

$(PREFIX_BUILD)/src/%.o:src/%.c $(BR_HEADERS) $(ADDITIONAL_HEADERS)
	$(CC) $(CCFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(PREFIX_BUILD)/src/%.o:src/%.cpp $(BR_HEADERS) $(ADDITIONAL_HEADERS)
	$(CC) $(CXXFLAGS) $(WARNING_FLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o:%.cpp $(ADDITIONAL_HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PREFIX_BUILD)/%.o:%.c $(ADDITIONAL_HEADERS)
	$(CC) $(CCFLAGS) -c -o $@ $<

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

bin/upper: tools/upper.cpp
	g++ -O3 -o bin/upper tools/upper.cpp

bin/lower: tools/lower.cpp
	g++ -O3 -o bin/lower tools/lower.cpp

src/misc/default_font.h: bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf
	bin/font_export fonts/PlayfairDisplayRegular-ywLOY.ttf > src/misc/default_font.h

bin/font_export: tools/font_export.c
	gcc -o bin/font_export tools/font_export.c -lm

$(SHADERS_HEADER): $(SHADERS_LIST) bin/upper bin/lower
	echo "" > $(SHADERS_HEADER)
	echo "#pragma once" >> $(SHADERS_HEADER)
	echo "//This file is autogenerated" >> $(SHADERS_HEADER)
	$(foreach s, $(SHADERS_LIST), echo "#define" | bin/lower >> $(SHADERS_HEADER) && \
																echo 'SHADER_$(word 4, $(subst /, , $(s))) \' | sed 's/\./_/' | bin/upper >> $(SHADERS_HEADER) && \
																cat $(s) | sed 's/\(.*\)/"\1\\n" \\/' >> $(SHADERS_HEADER) && \
																echo "" >> $(SHADERS_HEADER) && ) echo "OKi"
