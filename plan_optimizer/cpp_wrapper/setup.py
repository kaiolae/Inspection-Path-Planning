__author__ = 'kaiolae'

import os,sys,inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0,parentdir)

from distutils.core import setup, Extension
from settings.Constants_and_Datastructures import COMMON_SOURCES_FOLDER, MOEA_COVERAGE_FOLDER

"""
setup.py file for SWIG
"""

#Three things are essential when compiling the c++ extension:
#1. The location of headers (include_dirs)
#2, The name of external libraries (libraries)
#3. All source files we want to compile (sources)
eval_module = Extension('_cpp_binding',
                        include_dirs= [os.path.realpath(MOEA_COVERAGE_FOLDER), os.path.realpath(COMMON_SOURCES_FOLDER), "/usr/include/boost/"],
                        libraries = ['osg','osgViewer','osgSim','osgUtil','osgDB','osgGA','osgText', 'boost_serialization', 'boost_filesystem', 'OpenThreads'],
#, 'gsl',
                                     #'gslcblas', 'm', 'pthread',  'glpk', 'OpenThreads'], #'boost', 'emon',, gurobilib, 'gurobi_c++', 'GurobiJni60'
                           sources=['cpp_binding_wrap.cxx',os.path.realpath(MOEA_COVERAGE_FOLDER)+'/PlanCoverageEstimator.cpp',
                                    os.path.realpath(COMMON_SOURCES_FOLDER)+'/GeodeFinder.cpp',os.path.realpath(COMMON_SOURCES_FOLDER)+'/SceneKeeper.cpp',
                                   os.path.realpath(MOEA_COVERAGE_FOLDER)+'/PlanInterpreterBoxOrder.cpp',os.path.realpath(COMMON_SOURCES_FOLDER)+'/TriangleData.cpp',os.path.realpath(COMMON_SOURCES_FOLDER)+'/CameraEstimator.cpp',
                                    os.path.realpath(COMMON_SOURCES_FOLDER)+'/ImageViewerCaptureTool.cpp', os.path.realpath(COMMON_SOURCES_FOLDER)+'/KeyboardInputHandler.cpp',os.path.realpath(MOEA_COVERAGE_FOLDER)+'/ContourTracing.cpp'
                                    , os.path.realpath(COMMON_SOURCES_FOLDER)+'/OsgHelpers.cpp',os.path.realpath(MOEA_COVERAGE_FOLDER)+'/PlanEnergyEvaluator.cpp', os.path.realpath(COMMON_SOURCES_FOLDER)+'/HelperMethods.cpp']
                                    ,extra_compile_args=["-O2", "-std=c++11"] ,extra_link_args=["-O2"]#Enabling O2 optimization (think it is on by default too). See http://stackoverflow.com/questions/6928110/how-may-i-override-the-compiler-gcc-flags-that-setup-py-uses-by-default
                        )

setup (name = 'cpp_binding',
       version = '0.2',
       author      = "Kai Olav Ellefsen",
       description = """Python-to-C binding of an EA evaluation function estimating coverage by rendering scenes in OpenSceneGraph.""",
       ext_modules = [eval_module]
       )