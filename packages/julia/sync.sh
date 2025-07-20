#!/usr/bin/env sh
rsync root@10.169.1.4:/home/branimir/brplot-julia/Brplot/ ./Brplot_jll/ -r --stats -h -i
