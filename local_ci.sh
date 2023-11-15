#!/usr/bin/env sh

make PLATFORM=LINUX CONFIG=DEBUG GUI=IMGUI && \
make PLATFORM=LINUX CONFIG=DEBUG GUI=RAYLIB && \
make PLATFORM=LINUX CONFIG=DEBUG GUI=HEADLESS && \
make PLATFORM=LINUX CONFIG=RELEASE GUI=IMGUI && \
make PLATFORM=LINUX CONFIG=RELEASE GUI=RAYLIB && \
make PLATFORM=LINUX CONFIG=RELEASE GUI=HEADLESS && \
make PLATFORM=WINDOWS CONFIG=DEBUG GUI=IMGUI && \
make PLATFORM=WINDOWS CONFIG=DEBUG GUI=RAYLIB && \
make PLATFORM=WINDOWS CONFIG=DEBUG GUI=HEADLESS && \
make PLATFORM=WINDOWS CONFIG=RELEASE GUI=IMGUI && \
make PLATFORM=WINDOWS CONFIG=RELEASE GUI=RAYLIB && \
make PLATFORM=WINDOWS CONFIG=RELEASE GUI=HEADLESS && \
make PLATFORM=WEB CONFIG=DEBUG GUI=IMGUI && \
make PLATFORM=WEB CONFIG=DEBUG GUI=RAYLIB && \
make PLATFORM=WEB CONFIG=DEBUG GUI=HEADLESS && \
make PLATFORM=WEB CONFIG=RELEASE GUI=IMGUI && \
make PLATFORM=WEB CONFIG=RELEASE GUI=RAYLIB && \
make PLATFORM=WEB CONFIG=RELEASE GUI=HEADLESS && \
echo "Running fuzztests" && time cat /dev/random | ./bin/brplot_headless_linux_debug > /dev/null && echo "Fuzz test OK" && \
time ./bin/brplot_imgui_linux_debug --unittest && echo "UNIT TESTS for imgui OK" && \
time ./bin/brplot_raylib_linux_debug --unittest && echo "UNIT TESTS for raylib OK" && \
time ./bin/brplot_headless_linux_debug --unittest && echo "UNIT TESTS for headless OK" && \
ls -alFh bin && \
strip bin/* && \
ls -alFh bin && \
echo "OK"
