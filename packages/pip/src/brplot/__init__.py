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

class Brplot_Global_Counter:
    def __init__(self):
        self.value = 0

BrId = Brplot_Global_Counter()

def Private__is_number(a):
    return isinstance(a, float) or isinstance(a, int)
def Private__is_iterable(a):
    try:
        for _ in a:
            return True
    except _:
        return False
def Private__bad_arguments(a, b = None, c = None):
    raise Exception(f"Bad argument types: ({type(a)}){a}, ({type(b)}){b}, ({type(c)}){c}")

def plot(a, b = None, c = None):
    if Private__is_number(a): 
        if b is None and c is None:
            brplot.brp_1(ctypes.c_double(a), ctypes.c_int32(BrId.value))
        elif isinstance(b, int) and c is None:
            brplot.brp_1(ctypes.c_double(a), ctypes.c_int32(b))
            BrId.value = b
        elif Private__is_number(a) and Private__is_number(b) and isinstance(c, int):
            brplot.brp_2(ctypes.c_double(a), ctypes.c_double(b), ctypes.c_int32(c))
            BrId.value = c
        else:
            Private__bad_arguments(a, b, c)
    elif Private__is_iterable(a):
        if b is None and c is None:
            for x in a:
                brplot.brp_1(ctypes.c_double(x), ctypes.c_int32(BrId.value))
        elif Private__is_number(b) and c is None:
            for x in a:
                brplot.brp_1(ctypes.c_double(x), ctypes.c_int32(b))
            BrId.value = b
        elif Private__is_iterable(b) and c is None:
            for (x, y) in zip(a, b):
                brplot.brp_2(ctypes.c_double(x), ctypes.c_double(y), ctypes.c_int32(BrId.value))
        elif Private__is_iterable(b) and isinstance(c, int):
            for (x, y) in zip(a, b):
                brplot.brp_2(ctypes.c_double(x), ctypes.c_double(y), ctypes.c_int32(c))
            BrId.value = c
        else:
            Private__bad_arguments(a, b, c)
    else:
        Private__bad_arguments(a, b, c)

def label(label, id = None):
    if (isinstance(label, str)):
        bs = label.encode() + b'\0'
    elif(isinstance(label, bytes)):
        bs = label + b'\0'
    else:
        Private__bad_arguments(label, id)
        return

    if (id is None):
        brplot.brp_label(ctypes.c_char_p(bs), ctypes.c_int(BrId.value))
    elif (Private__is_number(id)):
        brplot.brp_label(ctypes.c_char_p(bs), ctypes.c_int(id))
        BrId.value = id
    else:
        Private__bad_arguments(label, id)

def flush():
    brplot.brp_flush()

def empty(group_id = None):
    if group_id is None:
        brplot.brp_empty(ctypes.c_int32(BrId.value))
    elif (Private__is_number(group_id)):
        brplot.brp_empty(ctypes.c_int32(group_id))
        BrId.value = group_id
    else:
        Private__bad_arguments(group_id)

def wait():
    brplot.wait()

def focus(a = None):
    brplot.brp_focus_all()

