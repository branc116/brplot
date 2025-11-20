del bin\brplot.exe
REM cl.exe /I. /Zi /DEBUG:FULL /Fe:bin\brplot.exe tools\unity\brplot.c
clang.exe -I. -o bin\brplot.exe tools\unity\brplot.c
bin\brplot.exe
