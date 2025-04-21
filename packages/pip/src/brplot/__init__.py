import ctypes
import os
import importlib
import site
from pathlib import Path


brplot = None
for s in site.getsitepackages():
    if brplot is not None:
        break
    for suffix in importlib.machinery.EXTENSION_SUFFIXES:
        try:
            brplot_path = f"{s}/brplot{suffix}"
            brplot = ctypes.cdll.LoadLibrary(brplot_path)
            break
        except:
            pass

if brplot is None:
    raise "Failed to load brplot dynamic library"


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

