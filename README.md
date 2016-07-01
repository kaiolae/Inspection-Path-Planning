The runnables are all inside the folder plan_optimizer/
See the README in that folder for details.

The optimization code is written in Python 2.

The most common way to use the planner will probably be the command:

./plan_optimizer/optimize_coverage.py -i INPUT_3D_MODEL -o OUTPUT_FOLDER

This will generate a plan for the given 3D model and store the results in the output folder. It will take some time, depending on the parameters defined for the optimization. The input model has to be compatible with OpenScenegraph. If you have problems, take a look at the README inside plan_optimizer/

-------

The code for optimizing inspection plans is written in Python, and that for evaluating plans is in C++.
A wrapper for the C++ code is auto-generated with the SWIG framework.
The file plan_optimizer/cpp_wrapper/cpp_binding.i defines the wrapper.
If you should need to modify the c++ code, you will need to re-generate the wrapper code. This is done automatically, if you execute the script
plan_optimizer/cpp_wrapper/compilation.sh
If you are changing the C++ interface that is exposed to Python in any way, you also have to update the wrapper definition file
plan_optimizer/cpp_wrapper/cpp_binding.i
