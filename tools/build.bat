REM del bin\brplot.exe

REM nob amalgam
REM clang.exe -ggdb -c -DBRUI_IMPLMENTATION -x c .generated/brui.h -o build/amal_brui.o
cl.exe /Zi -wd4201 /WX /W4 -DBRUI_IMPLMENTATION /DBR_DISABLE_LOG /Fe:bin/ui.exe /Iinclude /I. .\tests\test_ui.c
clang -Wconversion -Wall -Wextra -Wpedantic -Wfatal-errors -Werror -Wno-gnu-binary-literal -DBRUI_IMPLMENTATION -o bin/ui.exe -I include -I. tests/test_ui.c

cl.exe /I. /Zi /DEBUG:FULL /Fe:bin\brplot.exe tools\unity\brplot.c
clang.exe -Wfatal-errors -I. -o bin\brplot.exe tools\unity\brplot.c
bin\brplot.exe
