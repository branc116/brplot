#!/usr/bin/sh

NAME="brplot"
PROCES="$(ps a | grep -v make | grep -v grep | grep "$NAME" | tail -n 1)"
echo "$PROCES" && \
PID="$(echo $PROCES | awk '{print $1}')" && \
(test -z "$PID" && echo "$NAME not started") || \
  ( echo "Attaching to ($PID)" && \
    gdb --tui --silent -ex "attach $PID" )
