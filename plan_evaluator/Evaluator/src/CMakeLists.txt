cmake_minimum_required( VERSION 2.6 )
project( plan_evaluator )

#This allows me to define how to fetch some modules in another make-file.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

find_package(OpenThreads REQUIRED)
find_package(osg REQUIRED)
find_package(osgViewer REQUIRED)
find_package(osgSim REQUIRED)
find_package(osgUtil REQUIRED)
find_package(osgDB REQUIRED)
find_package(osgGA REQUIRED)
find_package(osgText REQUIRED)
find_package(Boost REQUIRED system serialization filesystem)

include_directories(
	${OSG_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

#Notice I'm using c++ 11.
SET(CMAKE_CXX_FLAGS "-Wall -std=c++11")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_DEBUG  "-Og -g") #Og is a special 

add_executable(
	moeaCoverage
	../../Utility_Functions/src/OsgHelpers.cpp
	../../Utility_Functions/src/HelperMethods.cpp
	../../Utility_Functions/src/SceneKeeper.cpp
	../../Utility_Functions/src/GeodeFinder.cpp
	../../Utility_Functions/src/CameraEstimator.cpp
	../../Utility_Functions/src/TriangleData.cpp
	../../Utility_Functions/src/ImageViewerCaptureTool.cpp
	../../Utility_Functions/src/KeyboardInputHandler.cpp
	moeaCoverageRunner.cpp
	PlanCoverageEstimator.cpp
	PlanInterpreterBoxOrder.cpp
	ContourTracing.cpp
	PlanEnergyEvaluator.cpp
)

target_link_libraries(
	moeaCoverage
	${OPENTHREADS_LIBRARY}
	${OSG_LIBRARY}
	${OSGVIEWER_LIBRARY}
	${OSGSIM_LIBRARY}
	${OSGUTIL_LIBRARY}
	${OSGDB_LIBRARY}
	${OSGGA_LIBRARY}
	${OSGTEXT_LIBRARY}
	${Boost_LIBRARIES}
)



