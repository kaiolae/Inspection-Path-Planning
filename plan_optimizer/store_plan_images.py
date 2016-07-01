#!/usr/bin/env python
import argparse
import os
import pickle
from os import listdir
from os.path import join, isdir
import pymsgbox
import yaml

import settings.Constants_and_Datastructures as Constants_and_Datastructures
from cpp_wrapper import cpp_binding
from evolutionary_operators.planEvaluatorSetup import preparePlanEvaluator
from settings import Utilities, Parameters

__author__ = 'kaiolae'

# Uses argparse to parse arguments, and returns the args object.
def parse_input_arguments():
    # Parsing user inputs:
    parser = argparse.ArgumentParser(
        description='Allows the user to export generated plans as images.\n'
                    'The user supplies the root folder of his results, plans are automatically extracted,\n'
                    'images generated and stored in a subfolder.\n')

    # Mandatory Arguments for all plan types
    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('input_folder', type=str,
                                     help='Path to folder containing your plans (this should be your results-folder,'
                                          ' which contains the sub-folder "populations").')
    parser.add_argument('--set_structure_viewpoint', action='store_true',
                        help='If given, the user can define a viewpoint for this structure, from which images of plans will be shown.'
                             ' If not given, an already stored viewpoint will be used when available.')
    args = parser.parse_args()
    return args


#Lets the user select a viewpoint to see plans for a given structure from.
def selectViewpoint(structurePath):

    pymsgbox.alert(title="Select camera location",
                                  text="To store images of generated plans, you need to decide the camera "
                                  "position that stored images will be shown from. This should ideally be "
                                  "a position that gives a good view of the entire structure. \n\n"
                                  "In the following scene, adjust the camera until you are satisfied."
                                  "Then press the w key to store the viewpoint, followed by esc to continue")
    # Reading in any existing viewpoints
    viewpointFile = open(Constants_and_Datastructures.VIEWPOINTS_FILE, 'w+')
    viewpointDict = yaml.load(viewpointFile)
    viewpointFile.close()

    if not viewpointDict:
        print "No viewpoint dict. creating new."
        viewpointDict = {}

    viewMatrix = cpp_binding.viewMatrixSelector(structurePath)
    if viewMatrix == ((1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)):
        answer = pymsgbox.confirm("You did not select a camera location to view the structure from. This will most likely "
                         "lead to pictures of the plan being difficult to interpret. Are you sure you want to continue?",
                         "WARNING: No camera location selected.", ["Yes","No"])
        if answer == "No":
            recursiveViewpoint = selectViewpoint(structurePath)
            return recursiveViewpoint

    # Without this conversion, the rows are not readable outside python.
    viewpointDict[structurePath] = [list(row) for row in viewMatrix]

    # Reopening viewpoint file in write-mode.
    viewpointFile = open(Constants_and_Datastructures.VIEWPOINTS_FILE, 'w')
    yaml.dump(viewpointDict,viewpointFile)
    viewpointFile.close()

    return viewMatrix

#Gets a viewpoint we already stored for a structure.
def getStoredViewpoint(structurePath):
    if not os.path.isfile(Constants_and_Datastructures.VIEWPOINTS_FILE):
        return False
    viewpointFile = open(Constants_and_Datastructures.VIEWPOINTS_FILE, 'r')
    viewpointDict = yaml.load(viewpointFile)
    if viewpointDict == None:
        viewpointDict = {}
    if structurePath in viewpointDict.keys():
        return viewpointDict[structurePath]
    else:
        return False

#Stores all the plans in the given file to a subfolder in the same folder (individualsFolder).
#For each plan, a snapshot/image is taken, and stored as a picture file. Allows us to quickly get an overview
#of all generated plans.
def storePlansToFolder( individualsFolder, individualsFile, storeFolder, locallyOptimizedFrom = False, force_camera_repositioning = False):
    '''
    Stores a group of plans to a folder, for easy inspection of all results.
    :type individualsFolder: str The folder we find the individuals to store
    :type individualsFile: str The file name of the file containing our individuals
    :type storeFolder: str The file we want to store the images from.
    :param locallyOptimizedFrom: (Optional). If given, gives the address of "original" individuals that the printed individuals were optimized from.
    This is useful, since the original individuals contain information on the parameters used during optimization.
    :return:
    '''

    if not os.path.exists(storeFolder):
        os.makedirs(storeFolder)

    if locallyOptimizedFrom:
        fileContainingParameters = locallyOptimizedFrom
    else:
        fileContainingParameters = individualsFolder+"/"+individualsFile

    #Loading data from evolutionary run
    try:
        loadedData = Utilities.load_dumped_population(fileContainingParameters)
    except IOError:
        print "No such file or Directory: ", fileContainingParameters
        return

    if locallyOptimizedFrom:
        population = pickle.load(open(individualsFolder+"/"+individualsFile, "rb"))
    else:
        population = loadedData[Constants_and_Datastructures.populationName]
    scene = loadedData[Constants_and_Datastructures.sceneName]

    #Getting the viewpoint to the scene
    #If viewpoint is stored, get it. If not, ask user for one.
    if force_camera_repositioning:
        selectViewpoint(scene)
    wp = getStoredViewpoint(scene)
    while wp==False:
        print "You did not store a camera viewpoint for this scene. Please align the camera as desired, press w to store the viewpoint, and then click esc to finish."
        wp = selectViewpoint(scene)
    print wp
    pco=preparePlanEvaluator(loadedData)


    counter = 0
    print "population size: ", len(population)
    if locallyOptimizedFrom:
        for name, plan in population.items():
            fileName = name + ".png"
            pco.storePlanImage(plan,wp,storeFolder+fileName)
    else:
        for p in population:
            fileName = "i"+str(counter)+"-fitness"
            print "Storing individual to ", storeFolder+fileName, ": ", p
            try:
                for v in p.fitness.values:
                    fileName+="_"+str(v)
            except AttributeError: # If we have a dict instead of an object here, we extract it this way.
                for v in p["fitness"]:
                    fileName+="_"+str(v)
            fileName+=".png"
            try:
                pco.storePlanImage(p,wp,storeFolder+fileName)
                print "plan image store"
            except TypeError:
                #The best may be to generate the population-object during loading of the yml.
                print "Plotting individual: ", p["genotype1"]
                pco.storePlanImage(p["genotype1"], wp, storeFolder+fileName)
            counter+=1

if __name__ == "__main__":
    #TODO: Start enabling all load methods to handle yml's instead of pickled files.
    args = parse_input_arguments()
    mainFolder = args.input_folder
    populationFolder = os.path.join(mainFolder, Constants_and_Datastructures.populationsSubFolder)
    imagesFolder = os.path.join(mainFolder,Constants_and_Datastructures.imagesSubFolder)
    seedImagesFolder = os.path.join(mainFolder, Constants_and_Datastructures.seedImagesSubFolder)

    storePlansToFolder(populationFolder, Constants_and_Datastructures.FINAL_POPULATION_FILE_YML, imagesFolder,  force_camera_repositioning=args.set_structure_viewpoint)
    storePlansToFolder(populationFolder, Constants_and_Datastructures.SEED_PLAN_FILE_YML, seedImagesFolder, force_camera_repositioning=args.set_structure_viewpoint)
