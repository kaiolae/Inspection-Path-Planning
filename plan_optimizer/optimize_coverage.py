#!/usr/bin/env python

__author__ = 'kaiolae'

import copy
import os
import random
import sys
import time
from functools import partial

import numpy
from deap import tools, base

from cpp_wrapper.cpp_binding import nothing #plotting_style
import plan_exporter as planExporter
import evolutionary_operators.EvaluationInterface
import evolutionary_operators.individuals
import evolutionary_operators.mutationAndRecombination
import settings.Constants_and_Datastructures
import settings.Utilities as Utilities
import settings.runtime_specified_parameters as runtime_specified_parameters
import store_plan_images as storePlanImages
from settings.Constants_and_Datastructures import FINAL_POPULATION_FILE_PKL, FINAL_POPULATION_FILE_YML, SEED_PLAN_FILE_PKL, SEED_PLAN_FILE_YML

#EA solution to coverage path planning problem - optimizing sensor positions and paths between them
#with the goals of minimizing travelling costs, and maximizing coverage percentage.


def evaluate(evaluator, memoization, override_energy_limit, individual):
    #A simple cleanup fixing individuals that may be unsuitable - for instance. by deleting consecutive identical viewpoints.
    evolutionary_operators.mutationAndRecombination.cleanUpIndividual(individual)
    return evaluator.evaluatePlan(individual, memoization, nothing, override_energy_limit)

def storeSeedFitnesses(seedIndividuals, toolbox, storageFolder,evaluator, elapsedTime):
    if len(seedIndividuals) == 0:
        return
    #Storing the fitness of seed-plans.
    fitnesses = toolbox.map(partial(toolbox.evaluate, evaluator, params.USING_EDGE_MEMOISATION, False), seedIndividuals)
    for ind, fit in zip(seedIndividuals, fitnesses):
        ind.fitness.values = fit
    Utilities.storePopulationAndParameters(seedIndividuals, SEED_PLAN_FILE_PKL, args.output_folder, args.input_model_path, params, elapsedTime=elapsedTime)
    Utilities.storePopulationAndParameters(seedIndividuals, SEED_PLAN_FILE_YML, args.output_folder, args.input_model_path, params, elapsedTime=elapsedTime)

def generateSeedIndividuals(evaluator):
    #The individuals we want to seed our population with.
    seedIndividuals = []
    if runtime_specified_parameters.use_seeds:
        seedPlans=evaluator.getSimplePlans()
        for plan in seedPlans:
            seed = [list(i) for i in plan]
            seedIndividuals.append(evolutionary_operators.individuals.Individual.initializeFromList(seed))

    else:
        print "No seeding chosen. Not applying any seeds."
        return [], 0

    elapsedTime = time.time() - runtime_specified_parameters.algorithm_start_time
    return seedIndividuals, elapsedTime


def initialize(toolbox,storageFolder, multirun):

    #The main object we use to interface to the c++ evalution of plans.
    evaluator = evolutionary_operators.EvaluationInterface.generateEvaluator(args.input_model_path, params.SENSOR_PARAMETERS,
                                                                             postProcessing=False, planLoopsAround=params.PLAN_LOOPS_AROUND, startLocation=params.PLAN_ORIGIN,
                                                                             printerFriendly=True)

    #After the C++ object has been set up, we query it for some parameter values that we will use later.
    runtime_specified_parameters.num_potential_viewpoints = evaluator.getNumberOfBoxes()
    runtime_specified_parameters.max_energy_usage = evaluator.getMaxAllowedEnergy()
    #All the seeds we are considering. Empty if we don't seed.
    seedIndividuals, elapsedTime = generateSeedIndividuals(evaluator)


    #Initializing the EA.
    random.seed()
    #Fitness function: Minimize distance, maximize weight.
    toolbox.register("evaluate",evaluate)

    #Writing custom population-initializer, since I want random-length individuals.
    def population(length):
        pop = []
        while len(pop)<length:
            if len(seedIndividuals) > 0 and random.random()<params.USE_PLAN_SEED_CHANCE:
                #Adding a randomly chosen seed from our list of potential seeds.
                pop.append(copy.deepcopy(random.choice(seedIndividuals)))
            else:
                #Adding a randomized individual. Re-sampling if the individual is not valid.
                valid_individual_generated=False
                while not valid_individual_generated:
                    randInd = evolutionary_operators.individuals.Individual.randInit(random.randint(params.MIN_PLAN_SIZE, params.MAX_PLAN_SIZE), runtime_specified_parameters.num_potential_viewpoints)
                    fitness = toolbox.evaluate(evaluator, params.USING_EDGE_MEMOISATION, False, randInd)
                    if fitness[1]<runtime_specified_parameters.max_energy_usage:
                        #Individual valid. Adding to population.
                        valid_individual_generated=True
                        pop.append(randInd)
        return pop

    toolbox.register("population",population)
    toolbox.register("select", tools.selNSGA2)

    toolbox.register("crossover", tools.cxMessyOnePoint) #Crossover for individuals of different length
    storeSeedFitnesses(seedIndividuals, toolbox, storageFolder, evaluator, elapsedTime)

    return evaluator


def main_nsga2(storageFolder, multirun):


    toolbox = base.Toolbox()
    evaluator = initialize(toolbox,storageFolder, multirun)

    print "numBoxes is ", runtime_specified_parameters.num_potential_viewpoints

    pf = tools.ParetoFront()
    random.seed()

    #Setting up stats and logging
    stats_fit = tools.Statistics(lambda ind: ind.fitness.values)
    stats_size = tools.Statistics(key=len) #Size of plans
    mstats = tools.MultiStatistics(fitness=stats_fit, size=stats_size)
    mstats.register("avg", numpy.mean, axis=0)
    mstats.register("std", numpy.std, axis=0)
    mstats.register("min", numpy.min, axis=0)
    mstats.register("max", numpy.max, axis=0)

    logbook = tools.Logbook()
    logbook.header = "gen", "evals", "fitness", "size"
    logbook.chapters["fitness"].header = "min", "avg", "max"
    logbook.chapters["size"].header = "min", "avg", "max"



    pop = toolbox.population(params.NUM_INDIVS)

    # Evaluate the individuals with an invalid fitness
    #This map, can also multiprocess to parallelize calculations.
    invalid_ind = [ind for ind in pop if not ind.fitness.valid]
    fitnesses = toolbox.map(partial(toolbox.evaluate, evaluator, params.USING_EDGE_MEMOISATION, False), invalid_ind)
    for ind, fit in zip(invalid_ind, fitnesses):
        ind.fitness.values = fit


    # This is just to assign the crowding distance to the individuals
    # no actual selection is done
    pop = toolbox.select(pop, len(pop))
    record = mstats.compile(pop)
    logbook.record(gen=0, evals=len(invalid_ind), **record)
    print(logbook.stream)


    # Begin the generational process
    for gen in range(0, params.NUM_GENERATIONS):

        genStart = time.time()

        print "Generation ", gen
        # Vary the population

        offspring = tools.selTournamentDCD(pop, len(pop))
        offspring = [toolbox.clone(ind) for ind in offspring]


        #Crossover and mutation - We keep fitness values for unmodified individuals - to avoid costly recalculations.
        if params.CROSSOVER_PROB>0:
            #Slicing array to "zip together" odd and even individuals
            for ind1, ind2 in zip(offspring[::2], offspring[1::2]):
                if random.random() <= params.CROSSOVER_PROB:
                    toolbox.crossover(ind1, ind2)
                    del ind1.fitness.values, ind2.fitness.values



        if params.WAYPOINT_MUTATION_PROBABILITY>0:
            for ind in offspring:
                if random.random() <= params.WAYPOINT_MUTATION_PROBABILITY:
                    didMutate = evolutionary_operators.mutationAndRecombination.mutate_waypoints(ind, toolbox)
                    if didMutate:
                        del ind.fitness.values

        if (not params.SIMPLIFIED_VIEW_ANGLE) and params.ANGLE_MUTATION_PROBABILITY>0:
            for ind in offspring:
                didMutate = evolutionary_operators.mutationAndRecombination.mutateViewingAngles(ind)
                if didMutate:
                    del ind.fitness.values

         #Reevaluating all individuals that were changed by crossover and mutation.
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitnesses = toolbox.map(partial(toolbox.evaluate, evaluator, params.USING_EDGE_MEMOISATION, False), invalid_ind)
        for ind, fit in zip(invalid_ind, fitnesses):
            ind.fitness.values = fit

        # Select the next generation population from current population and valid offspring. Invalid offspring are those
        # that became cropped too much by crossover.
        valid_offspring = [o for o in offspring if len(o)>=params.MIN_PLAN_SIZE]
        pop = toolbox.select(pop + valid_offspring, params.NUM_INDIVS)
        record = mstats.compile(pop)
        logbook.record(gen=gen, evals=len(invalid_ind), **record)
        print(logbook.stream)

        if(params.STORAGE_INTERVAL!=-1 and gen%params.STORAGE_INTERVAL==0):
            Utilities.storePopulationAndParameters(pop,"gen"+str(gen)+".pkl", args.output_folder, args.input_model_path, params,)
            Utilities.storePopulationAndParameters(pop,"gen"+str(gen)+".yml", args.output_folder, args.input_model_path, params,)
            #awplotFitness(logbook,block=False)

        pf.update(pop)
        #Updating the memoized values, to avoid filling up memory.
        #Could consider doing this only each X generation - to save time.
        if params.USING_EDGE_MEMOISATION:
            runtime_specified_parameters.num_memoized_edges = evaluator.updateMemoisedEdges(pop)
    return pop, logbook, pf




if __name__ == "__main__":

    # Parsing command line arguments
    args = Utilities.parse_inspection_planning_arguments()
    storageFolder = args.output_folder
    if args.disable_seeding:
        runtime_specified_parameters.use_seeds = False

    runtime_specified_parameters.algorithm_start_time = time.time()

    if args.multirun:
        #In multirun, we load the settings-file from the storage folder, to ensure settings do not change during run.
        #In fact, the settings file is located one level above the storage folder, hence the ../
        params = Utilities.importFromURI(args.output_folder + "../experiment_parameters/Parameters.py")
        print "MULTIRUN. Fetching parameters from ", params
    else:
        import settings.Parameters as params
        if not args.no_post_processing:
            # If we do postprocessing, we need to select a viewpoint to store plan pictures from. The user selects that,
            # unless one is already stored for this specific 3D model.
            if args.set_structure_viewpoint or not storePlanImages.getStoredViewpoint(args.input_model_path):
                storePlanImages.selectViewpoint(args.input_model_path)

    #Ensuring the same params are available to every part of the program
    runtime_specified_parameters.params = params
    pop, stats, paretoFront = main_nsga2(storageFolder, args.multirun)
    print "Pareto front size is ", paretoFront
    print "Fitness frontier:"
    for p in pop:
        print p.fitness.values

    #Temporarily storing results as both pkl and yml, since I'm transitioning from pkl to yml.
    Utilities.storePopulationAndParameters(pop, FINAL_POPULATION_FILE_PKL, args.output_folder, args.input_model_path, params, paretoFront, time.time() - runtime_specified_parameters.algorithm_start_time)
    Utilities.storePopulationAndParameters(pop, FINAL_POPULATION_FILE_YML, args.output_folder, args.input_model_path, params, paretoFront, time.time() - runtime_specified_parameters.algorithm_start_time)

    #  If we are doing a normal run (not a multirun), we store all the generated plans as yml-files and pictures.
    # TODO: Consider storing other things too: pareto fronts, etc. Or are they better to produce in post-analysis?
    if not args.multirun and not args.no_post_processing:
        # Building the paths to folders where we will store plans.
        population_folder = os.path.join(args.output_folder,
                                         settings.Constants_and_Datastructures.populationsSubFolder)
        image_store_folder = os.path.join(args.output_folder,
                                          settings.Constants_and_Datastructures.imagesSubFolder)
        seed_images_folder = os.path.join(args.output_folder, settings.Constants_and_Datastructures.seedImagesSubFolder)
        yml_store_folder = os.path.join(args.output_folder, settings.Constants_and_Datastructures.ymlSubfolder)
        #Store images to separate subfolder
        storePlanImages.storePlansToFolder(population_folder, FINAL_POPULATION_FILE_PKL, image_store_folder)
        storePlanImages.storePlansToFolder(population_folder, settings.Constants_and_Datastructures.SEED_PLAN_FILE_YML, seed_images_folder)

        #Store yml-files to separate subfolder
        planExporter.exportAllWinnerIndividuals(args.output_folder)


