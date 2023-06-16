set -ex

$EMSCRIPTEN/emcc --closure 1 -s ENVIRONMENT=web -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2  -Wall -Wpedantic -Wextra -Os -I$RAYLIB/src -o build/graph_web.o -c graph.c
$EMSCRIPTEN/emcc --closure 1 -s ENVIRONMENT=web -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -s USE_GLFW=3 -s ASYNCIFY -Wall -Wpedantic -Wextra -Os -I$RAYLIB/src -o build/main_web.o -c main.c
$EMSCRIPTEN/emcc --closure 1 -s ENVIRONMENT=web -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -s USE_GLFW=3 -s ASYNCIFY -Wall -Wpedantic -Wextra -Os -I$RAYLIB/src -o build/refresh_shaders_web.o -c web_specific/refresh_shaders.c
$EMSCRIPTEN/emcc --closure 1 -s ENVIRONMENT=web -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -s USE_GLFW=3 -s ASYNCIFY -Wall -Wpedantic -Wextra -Os -I$RAYLIB/src -o build/udp_web.o -c web_specific/udp.c
$EMSCRIPTEN/emcc --memory-init-file 1 --llvm-lto 1 --closure 1 -s WASM_BIGINT -s ENVIRONMENT=web -Os -flto --shell-file=web_specific/minshell.html --preload-file shaders/web/grid.fs@shaders/grid.fs --preload-file shaders/web/line.fs@shaders/line.fs --preload-file shaders/web/line.vs@shaders/line.vs -sALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASYNCIFY \
  -o www/index.html build/graph_web.o build/main_web.o build/refresh_shaders_web.o build/udp_web.o $RAYLIB/build/raylib/libraylib.a
