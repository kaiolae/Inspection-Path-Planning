__author__ = 'kaiolae'

import argparse
import pickle
from os.path import dirname, join
from evolutionary_operators.planEvaluatorSetup import preparePlanEvaluator
import settings.Constants_and_Datastructures
import settings.Utilities as Utilities
from cpp_wrapper.cpp_binding import normal, circulating

def plotIndividual(base_folder, indivNr, fileName = "winner_indivs.pkl", circulating_plot = False):
    populationFile = join(join(base_folder,settings.Constants_and_Datastructures.populationsSubFolder),fileName)
    loadedData = pickle.load(open(populationFile, "rb"))
    population = loadedData[settings.Constants_and_Datastructures.populationName]
    plot_func = preparePlanEvaluator(loadedData)

    plottedIndiv = population[indivNr]
    print "Plotted individual is ", plottedIndiv
    print "Fitness is ", plottedIndiv.fitness.values

    if circulating_plot:
        plot_type = circulating
    else:
        plot_type = normal

    print "Plot type is ", plot_type

    (obj1,obj2) = plot_func.evaluatePlan(plottedIndiv,False, plot_type)
    print "Scores: ", obj1, ", ", obj2

def printFitnesses(loadFile):
    loadedData = pickle.load(open(loadFile, "rb"))
    population = loadedData[settings.Constants_and_Datastructures.populationName]
    for p in population:
        print p.fitness.values

def parse_input_arguments():
    # Parsing user inputs:
    parser = argparse.ArgumentParser(
        description='Allows the user to visualize a plan in 3D, in order to study it thoroughly.')

    # Mandatory Arguments
    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('-i','--input_folder', type=str, required=True,
                                     help='Path to folder containing your plans (this should be your results-folder,'
                                          ' which contains the sub-folder "populations").')


    parser.add_argument('--id', type = int, required=True, help='The ID number of the plan you would like to plot. This number can be found at the beginning of'
                                     " the filename of the image of the plan. It is a good idea to generate plan images first, and then"
                                     " study plans in more detail with this visualizer when necessarry. The ID is an integer, between 0 "
                                     "and the number of solutions generated by the optimizer.")
    #Optional arguments
    parser.add_argument('--circulating', action='store_true',
                        help='If given, the plan is shown by the camera automatically circling around the structure,'
                             'instead of the user controlling the camera.')

    args = parser.parse_args()
    return args

if __name__ == "__main__":

    args = parse_input_arguments()
    base_folder = args.input_folder
    indivNr = args.id
    paramsFile = join(base_folder,join(settings.Constants_and_Datastructures.parametersSubFolder,"Parameters.py"))
    params = Utilities.importFromURI(paramsFile)

    loadFile = "winner_indivs.pkl"
    print "Circulating was ", args.circulating
    plotIndividual(base_folder, indivNr, loadFile, args.circulating)