#Requirements:

* Python 2: https://www.python.org/downloads/
* pip : https://pip.pypa.io/en/stable/installing/
* swig: http://www.swig.org/download.html

#Quick Install

To install the program, run

     sudo ./install.py

in the root folder of the project. Python is required.

#Installation

If the quick install should fail, you can install the required components yourself as described below.

The plan optimizer has a c++ and a Python component. The c++ component needs to be compliled first, and a wrapper needs to be generated for the Python interface. This is done automatically by the following command:

    sh plan_optimizer/cpp_wrapper/compilation.sh

After this, all that remains is installing your relevant Python dependencies. When running a Python script, you may see an error such as

    "ImportError: No module named 'ModuleName'"

This can be fixed by installing the relevant module with the command

    sudo pip install ModuleName

#Running

The runnables are all inside the folder plan_optimizer/
See the README in that folder for details.

The optimization code is written in Python 2.

The most common way to use the planner will probably be the command:

    ./plan_optimizer/optimize_coverage.py -i INPUT_3D_MODEL -o OUTPUT_FOLDER

This will generate a plan for the given 3D model and store the results in the output folder. It will take some time, depending on the parameters defined for the optimization. The input model has to be compatible with OpenScenegraph. If you have problems, take a look at the README inside plan_optimizer/


#Dependency-free running

If you prefer to not install any Python libraries, and you will not modify any code, the basic planning functionality is available in the executable
    executable/optimize_coverage/optimize_coverage

It takes exactly the same arguments as optmize_coverage.py (see above).


#Code structure

The code for optimizing inspection plans is written in Python, and that for evaluating plans is in C++.
A wrapper for the C++ code is auto-generated with the SWIG framework.
The file plan_optimizer/cpp_wrapper/cpp_binding.i defines the wrapper.
If you should need to modify the c++ code, you will need to re-generate the wrapper code. This is done automatically, if you execute the script
plan_optimizer/cpp_wrapper/compilation.sh
If you are changing the C++ interface that is exposed to Python in any way, you also have to update the wrapper definition file
plan_optimizer/cpp_wrapper/cpp_binding.i

#Parameters

The parameters of the optimization can be modified in the file

    plan_optimizer/settings/Parameters.py

It helps to have a certain knowledge of evolutionary algorithms to modify this.

Parameters of the simulated robot, including collision buffers, camera parameters, etc. are all set in the file

    plan_evaluator/Utility_Functions/Constants.h

If you modify this, make sure you recompile the c++ code before running the optimizer. That is done with the command

    sh plan_optimizer/cpp_wrapper/compilation.sh
