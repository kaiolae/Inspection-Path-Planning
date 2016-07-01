import ConfigParser

__author__ = 'kaiolae'

import os
from deap import base

#Some constants that may be useful to access in several scripts.
populationName = "population"
sceneName = "scene"
originName = "plan_origin"
sensorParamsName = "sensor_params"
paretoFrontName = "pareto_front"
maxEnergyName = "max_energy"
elapsedTimeName = "elapsed_time"
planLoopsAroundName = "plan_loops_around"
numMemoizedSolutionsName = "num_memoized_solutions"

# Things we may want to plot
hyperVolumeName = "hypervol"
lengthName = "length"

SOURCE_CODE_ROOT = os.path.split(os.path.abspath(os.path.dirname(__file__)))[0]
RESULTS_ROOT = os.path.join(os.path.split(SOURCE_CODE_ROOT)[0],"results")
# Paths to subfolders of the base path defined config file.
SYSTEM_CONFIGURATIONS = ConfigParser.ConfigParser()
SYSTEM_CONFIGURATIONS.read(os.path.join(SOURCE_CODE_ROOT,"settings/project_config.ini"))

SETTINGS_DIR = os.path.dirname(__file__)
COVERAGE_ESTIMATOR_PROJECT_BASE = os.path.join(SETTINGS_DIR,'/home/kai/inspection_planner/plan_evaluator')#'../../plan_evaluator/')
MOEA_COVERAGE_FOLDER = os.path.join(COVERAGE_ESTIMATOR_PROJECT_BASE, 'Evaluator/src/')
COMMON_SOURCES_FOLDER = os.path.join(COVERAGE_ESTIMATOR_PROJECT_BASE, 'Utility_Functions/src/')
PATH_TO_CONSTANTS_FILE = os.path.join(COMMON_SOURCES_FOLDER, "Constants.h")

# A single file where we store camera viewpoints for each model - pnly used for plotting.
VIEWPOINTS_FILE = os.path.join(SETTINGS_DIR,'../../3d_models/viewpoints.yml')

#Relative paths to subfolders where some results are stored
populationsSubFolder = "populations/"
parametersSubFolder = "experiment_parameters/"
imagesSubFolder = "plan_images/"
seedImagesSubFolder = "seed_images/"
ymlSubfolder = "plan_ymls/"


# Datastructures

class Fitness(base.Fitness):
    weights = (-1,-1) # Weighting the two objectives equally


FINAL_POPULATION_FILE_PKL = "winner_indivs.pkl" # Pickle-file to store the winner individuals
FINAL_POPULATION_FILE_YML = "winner_indivs.yml" # YML-file to store the winner individuals
SEED_PLAN_FILE_PKL = "base_plans.pkl"
SEED_PLAN_FILE_YML = "base_plans.yml"


CIRCLING_PLANS_FILE_YML = "circling_plans.yml"
CIRCLING_PLANS_FILE_PKL = "circling_plans.pkl"

#Constants used for plotting

#Paths to some locations where generated plans and statistics are stored - used in plotting:
MOEA_SEEDED_SUBFOLDER = "moea_seeded"
MOEA_NO_SEED_SUBFOLDER = "moea_noseed"
CIRCLING_PLANS_FILE_PATH = "circling/populations/circling_plans.pkl"


SEEDED_COLOR = "#95CF80"
UNSEEDED_COLOR = "#4d90c7"
SEEDED_MARKER = "^"
UNSEEDED_MARKER = "o"

