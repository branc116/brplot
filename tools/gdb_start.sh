#!/usr/bin/sh

gdb --tui --silent -ex "b br_dagen_handle" -ex "source tools/gdb.py" -ex "r" ./bin/brplot
