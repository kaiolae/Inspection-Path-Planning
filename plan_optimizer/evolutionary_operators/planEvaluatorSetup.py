import os

import EvaluationInterface
import settings.Constants_and_Datastructures
import settings.Utilities
from settings import Parameters

__author__ = 'kaiolae'


#  Prepares the evaluator-object, and returns it, ready for interpreting and evaluating plans.
#  This is meant for usage in post-processing - that is, after the EA has finished.
def preparePlanEvaluator(loadedData, post_processing = True):

    scene = loadedData[settings.Constants_and_Datastructures.sceneName]
    origin = loadedData[settings.Constants_and_Datastructures.originName]
    sensorParams = loadedData[settings.Constants_and_Datastructures.sensorParamsName]
    planLoopsAround = False
    if settings.Constants_and_Datastructures.planLoopsAroundName in loadedData.keys() and loadedData[
        settings.Constants_and_Datastructures.planLoopsAroundName]:
        planLoopsAround = True

    plot_func = EvaluationInterface.generateEvaluator(scene, sensorParams, postProcessing=post_processing,
                                                      startLocation=origin, planLoopsAround=planLoopsAround, printerFriendly=False)
    return plot_func