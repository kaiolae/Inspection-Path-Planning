#!/usr/bin/env bash
swig -Wall -python -c++ cpp_binding.i
OPT="" CC="gcc" python setup.py build_ext  --inplace #OPT removes all distutils' default compiler flags. Using ccache speeds up compilation when only a few source files have been changed.