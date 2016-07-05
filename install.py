#!/usr/bin/env python
#Installation is dependent on the pip and swig packages. If not installed, please install them as described at:
#https://pip.pypa.io/en/stable/installing/
#http://www.swig.org/download.html
import sys
import subprocess
def install(package):
    pip.main(['install', package])

def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None

pip_import_success = False
try:
    import pip
    print "PIP found"
    pip_import_success = True
except ImportError:
    sys.stderr.write("The python package manager pip is missing from your system. See install instructions at https://pip.pypa.io/en/stable/installing/")


if which("swig"):
    print "SWIG found"
    subprocess.call(['./plan_optimizer/cpp_wrapper/compilation.sh'])
else:
    sys.stderr.write("SWIG is missing from your system. This is needed to generate the c++ wrapper. See install instructions at http://www.swig.org/download.html")


if pip_import_success:
    with open("plan_optimizer/python_dependencies.txt", 'r') as required_packages:
        for line in required_packages:
            install(line)