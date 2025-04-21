from setuptools import setup, Extension
brplot_module = Extension("brplot", ["src/brplot/brplot.c"], define_macros=[("BRPLOT_IMPLEMENTATION", None), ("BR_PYTHON_BULLSHIT", None), ("BR_DISABLE_LOG", None)])
setup(ext_modules=[brplot_module])
