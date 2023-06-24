set -ex

$EMSCRIPTEN/emcc -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -Wall -Wpedantic -Wextra --memory-init-file 1 --closure 1 -s WASM_BIGINT -I$RAYLIB/src -s ENVIRONMENT=web -flto -Os --shell-file=web_specific/minshell.html --preload-file shaders/web/grid.fs@shaders/grid.fs --preload-file shaders/web/line.fs@shaders/line.fs --preload-file shaders/web/line.vs@shaders/line.vs -sALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASYNCIFY -o www/index.html graph.c main.c web_specific/refresh_shaders.c web_specific/udp.c smol_mesh.c points_group.c $RAYLIB/build/raylib/libraylib.a
