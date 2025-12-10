del bin\brplot.exe

nob amalgam
clang.exe -ggdb -c -DBRUI_IMPLMENTATION -x c .generated/brui.h -o build/amal_brui.o
clang.exe -ggdb -DBR_DISABLE_LOG -o bin/ui -I.generated ./tests/test_ui.c build/amal_brui.o

REM cl.exe /I. /Zi /DEBUG:FULL /Fe:bin\brplot.exe tools\unity\brplot.c
REM clang.exe -Wfatal-errors -I. -o bin\brplot.exe tools\unity\brplot.c
REM bin\brplot.exe
