#!/usr/bin/sh

gdb --tui --silent -ex "b main" -ex "r" ./brplot
