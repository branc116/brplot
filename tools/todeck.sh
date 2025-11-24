rsync -av --stats -h -i ./nob.c deck@192.168.0.25:/home/deck/Documents/brplot/nob.c
rsync -av -r --stats -h -i ./tools/ deck@192.168.0.25:/home/deck/Documents/brplot/tools/
rsync -av -r --stats -h -i ./external/ deck@192.168.0.25:/home/deck/Documents/brplot/external/
rsync -av -r --stats -h -i ./src/ deck@192.168.0.25:/home/deck/Documents/brplot/src/
# ssh deck@192.168.0.25 -C "bash -c 'cd /home/deck/Documents/brplot; ./nob'"
# rsync --stats -h -i ./src/platform2.c deck@192.168.0.25:/home/deck/Documents/brplot/src/platform2.c
