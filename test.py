import ctypes
import os
from pathlib import Path


if os.name == "posix":
    brplot_path = (Path(__file__) / ".." / "brplot.so").resolve()
    brplot = ctypes.cdll.LoadLibrary(brplot_path)
elif os.name == "windows":
    brplot_path = (Path(__file__) / ".." / "brplot.dll").resolve()
    brplot = ctypes.cdll.LoadLibrary(brplot_path)
else:
    print(f"Unknown os: {os.name}")
    exit(1)


class Private:
    def is_number(a):
        return isinstance(a, float) or isinstance(a, int)
    def is_iterable(a):
        try:
            for _ in a:
                return True
        except _:
            return False
    def bad_arguments(a, b, c):
        raise Exception(f"Bad argument types: ({type(a)}){a}, ({type(b)}){b}, ({type(c)}){c}")

def plot(a, b = None, c = None):
    if Private.is_number(a): 
        if b is None and c is None:
            brplot.brp_1(ctypes.c_float(a), ctypes.c_int32(0))
        elif isinstance(b, int) and c is None:
            brplot.brp_1(ctypes.c_float(a), ctypes.c_int32(b))
        elif Private.is_number(a) and Private.is_number(b) and isinstance(c, int):
            brplot.brp_2(ctypes.c_float(a), ctypes.c_float(b), ctypes.c_int32(c))
        else:
            Private.bad_arguments(a, b, c)
    elif Private.is_iterable(a):
        if b is None and c is None:
            for x in a:
                brplot.brp_1(ctypes.c_float(x), ctypes.c_int32(0))
        elif Private.is_number(b) and c is None:
            for x in a:
                brplot.brp_1(ctypes.c_float(x), ctypes.c_int32(b))
        elif Private.is_iterable(b) and c is None:
            for (x, y) in zip(a, b):
                brplot.brp_2(ctypes.c_float(x), ctypes.c_float(y), ctypes.c_int32(0))
        elif Private.is_iterable(b) and isinstance(c, int):
            for (x, y) in zip(a, b):
                brplot.brp_2(ctypes.c_float(x), ctypes.c_float(y), ctypes.c_int32(c))
        else:
            Private.bad_arguments(a, b, c)
    else:
        Private.bad_arguments(a, b, c)

def flush():
    brplot.brp_flush()

def empty(group_id):
    brplot.brp_empty(ctypes.c_int32(group_id))

def wait():
    brplot.wait()

def focus(a = None):
    brplot.brp_focus_all()

import math
freq = 30
n = 1000
offset = 0
plot(1)
plot(2)
plot(3)
plot(3, 3)
plot([1, 2, 3], 3)
plot([1, 2, 3], [3, 7, 9])
plot(range(10), [math.sin(x/10)*0.1 for x in range(1000)])
for i in range(n):
    x = (i-n/2) / 100
    plot(x, 0.1*math.sin(freq*x + offset), 1)
focus()
wait()

# clang -DBRPLOT_IMPLEMENTATION -shared -fpie -fPIC -o brplot.so .generated/brplot.c
# clang -DBRPLOT_IMPLEMENTATION -shared -o brplot.dll .generated/brplot.c
