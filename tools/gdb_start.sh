#!/usr/bin/sh

gdb --tui --silent -ex "file bin/brplot" -ex "b br_resampling_draw" -ex "source tools/gdb.py" -ex "r" ./bin/brplot
