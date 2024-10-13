with open("BPS") as file:
    for line in file.readlines():
        gdb.Breakpoint(line)
        print(line)

gdb.parse_and_eval("display i")
# gf2 --command=tools/gdb_debug.py ./a.out
