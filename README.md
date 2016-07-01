The runnables are all inside the folder plan_optimizer/
See the README in that folder for details.

The optimization code is written in Python 2.

-------

The code for optimizing inspection plans is written in Python, and that for evaluating plans is in C++.
A wrapper for the C++ code is auto-generated with the SWIG framework.
The file plan_optimizer/cpp_wrapper/cpp_binding.i defines the wrapper.
If you should need to modify the c++ code, you will need to re-generate the wrapper code. This is done automatically, if you execute the script
plan_optimizer/cpp_wrapper/compilation.sh
If you are changing the C++ interface that is exposed to Python in any way, you also have to update the wrapper definition file
plan_optimizer/cpp_wrapper/cpp_binding.i
