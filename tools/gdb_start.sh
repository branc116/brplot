#!/usr/bin/sh

gdb --tui --silent -ex "file bin/brplot" -ex "b br_series_read" -ex "source tools/gdb.py" -ex "r" ./bin/brplot
