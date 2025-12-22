#!/usr/bin/env sh


if [ -z "$(git status --porcelain)" ]; then
 exit 0
else
 exit 1
fi
