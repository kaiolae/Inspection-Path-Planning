__author__ = 'kaiolae'

from cpp_wrapper import cpp_binding

# An interface to make my Python and C++ code less dependent.

def generateEvaluator(sceneFile, sensorParams, postProcessing = False, startLocation = None,
                      planLoopsAround = False,
                      printerFriendly = False):


    evaluator = cpp_binding.PlanCoverageEstimator(sceneFile, sensorParams, postProcessing, startLocation,
                                                planLoopsAround, printerFriendly)
    return evaluator
