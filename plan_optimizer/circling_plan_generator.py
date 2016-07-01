#!/usr/bin/env python
import os
import time

from settings import Utilities
import settings.Parameters as params
import settings.Constants_and_Datastructures as consts
from evolutionary_operators import EvaluationInterface, individuals
from cpp_wrapper.cpp_binding import normal, circulating, image, nothing
from store_plan_images import storePlansToFolder, getStoredViewpoint

#This script generates a simple plan circling around the inspection target.
#These are normally used as seeds for optimization, but with this script, one could also generate them for other purposes.

if __name__ == "__main__":

    start_time = time.time()
    args = Utilities.parse_circling_planning_arguments()
    print "Outfolder: ", args.output_folder
    Utilities.store_run_info(args.output_folder)

    evaluator = EvaluationInterface.generateEvaluator(args.input_model_path, params.SENSOR_PARAMETERS,
                                                                        postProcessing=False,
                                                                        planLoopsAround=params.PLAN_LOOPS_AROUND,
                                                                        startLocation=params.PLAN_ORIGIN,
                                                                        printerFriendly=True)
    circling_plans = evaluator.getSimplePlans()
    circling_individuals = [individuals.Individual.initializeFromList(p) for p in circling_plans]
    fitnesses = [evaluator.evaluatePlan(p, False, nothing) for p in circling_plans] #TODO Change from nothing to normal or circling to show on screen.
    for ind, fit in zip(circling_individuals, fitnesses):
        ind.fitness.values = fit
    elapsedTime = time.time() - start_time

    #Storing run data
    Utilities.storePopulationAndParameters(circling_individuals, consts.CIRCLING_PLANS_FILE_PKL, args.output_folder, args.input_model_path,
                                 params, paretoFront = None, elapsedTime = elapsedTime)
    Utilities.storePopulationAndParameters(circling_individuals, consts.CIRCLING_PLANS_FILE_YML, args.output_folder, args.input_model_path,
                                 params, paretoFront = None, elapsedTime = elapsedTime)

    print "Generation done. Starting to store images."
    #Generating and storing images
    imagesSubFolder = os.path.join(args.output_folder, consts.imagesSubFolder)
    popSubFolder = os.path.join(args.output_folder, consts.populationsSubFolder)
    storePlansToFolder(popSubFolder,consts.CIRCLING_PLANS_FILE_PKL,imagesSubFolder)