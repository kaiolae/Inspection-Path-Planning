#!/usr/bin/env python
from settings import Utilities

__author__ = 'kaiolae'
import math
import os
import pickle
import sys
import yaml # From package PyYAML
import argparse
import numpy as np

import settings.Constants_and_Datastructures as Constants_and_Datastructures
import evolutionary_operators.planEvaluatorSetup

from settings.Utilities import generate_rotated_vectors_around_z, generate_simplified_roll_pitch_yaw

#Adds the offsets given by offsetstuple to each element of the sublist.
#Example: If listOfWaypoints = [[1,2,3],[4,5,6]] and offsetsTuple = (-1,0,1)
#the result will be [[0,2,4],[3,5,7]].
def addOffsetToEachSublist(originalListofLists,offsetsTuple):
    listWithOffset = []
    for sublist in originalListofLists:
        newSublist = []
        for (elem, offset) in zip(sublist,offsetsTuple):
            newSublist.append(elem+offset)
        listWithOffset.append(newSublist)

    return listWithOffset



# Splits any waypoints resulting in turning more sharply than a given max-turn-angle.
# This can give bettern 3D reconstructing, as we don't rotate so much that the target is lost from sight.
# The split generates two new waypoints instead of the old one, one split_distance metres before it, the other
# split_distance after. The new edge is also assigned a new orientation, as the average of the orientation before and after.
def split_sharply_turning_waypoints(plannedPositions, plannedOrientations, min_turn_angle=0.785398, split_distance = 4.0):
    newWaypoints = []
    newOrientations = []
    #PlannedOrientations[N] is the orientation on the way to plannedPositions[N+1]
    #The first waypoint will never be split
    newWaypoints.append(plannedPositions[0])
    for edge_counter in range(len(plannedOrientations)-1): #-1 since we will never split the last waypoint.
        orientation_before = plannedOrientations[edge_counter]
        orientation_after = plannedOrientations[edge_counter+1]
        start_wp = np.array(plannedPositions[edge_counter])
        end_wp = np.array(plannedPositions[edge_counter+1])
        next_wp = np.array(plannedPositions[edge_counter+2]) #The waypoint after next.
        incoming_vector = end_wp - start_wp
        outgoing_vector = next_wp - end_wp

        #Finding the angle between the two vectors
        angle = Utilities.angle_between(incoming_vector,outgoing_vector)
        print "Angle nr ", edge_counter, " was ", angle, ". Too steep? ", angle > abs(min_turn_angle)
        if angle > abs(min_turn_angle):
            new_wp_before, new_wp_after, new_orientation_between = \
                split_waypoint(incoming_vector, outgoing_vector, end_wp, orientation_before, orientation_after, split_distance)

            #Insert the new waypoints and orientation in place of the old ones.
            #TODO: Here, I have the old insert-while-iterating problem-.
            newWaypoints.append(new_wp_before) #Inserts the new "before-wp"
            newWaypoints.append(new_wp_after) #Inserts the new "after-wp"

            #Inserts the orientation on the way to the split waypoint
            newOrientations.append(orientation_before)
            #Inserts the orientation between the two new waypoints.
            newOrientations.append(new_orientation_between)


            edge_counter-=1 #Going back to the previous waypoint, in case it has to be split again.
        else:
            newWaypoints.append(end_wp)
            newOrientations.append(orientation_before)

    #The last waypoint will never be split
    newWaypoints.append(plannedPositions[-1])
    newOrientations.append(plannedOrientations[-1]) #The final orientation will always be maintained - since we don't split the final wp.

    print "Num new wps: ", len(newWaypoints), ". Num new angles: ", len(newOrientations)

    return newWaypoints, newOrientations


def convert_pair_of_lists_to_dict(key_list, value_list):
    '''
    Converts a pair of lists to a dict, by treating the first list as the keys and the second
    as the values.
    :param key_list: List that will become keys in the dict
    :param value_list: List that will become values in the dict
    :return: The resulting dictionary.
    '''
    generated_dict = {key_list[i]: value_list[i] for i in range(len(key_list))}
    return generated_dict


#Exports plans as YAML files. For now, we have agreed on the following format:
#offset:
#waypoints:
#  - waypoint: {x: 0, y: 0, z: 0}
#    orientation: {r: 0, p: 0, y: 0}
#  - waypoint: {x: 0, y: 0, z: 0}
#    orientation: {r: 0, p: 0, y: 0}
#  - waypoint: {x: 0, y: 0, z: 0}
#    orientation: {r: 0, p: 0, y: 0}
# The first waypoint is the starting point of the inspection - and the heading to that point is therefore not optimized.
# For all other waypoints, the heading to follow to that waypoint is specified in roll, pitch, yaw (r,p,y).

def exportPlanToYaml(planPath, yamlPath, indivNr, positionOffsets = None, rotationDegreesAroundzAxis = 0, evaluation_function = None,
                     min_turn_angle = None):

    if len(planPath) == 0:
        print "Trying to export empty plan."
        return
    print "Rotation argument: ", rotationDegreesAroundzAxis
    print "position offset argument: ", positionOffsets

    with open(planPath, "rb") as f:
        loadedData = pickle.load(f)
    population = loadedData[Constants_and_Datastructures.populationName]
    if evaluation_function == None:
        evaluation_function = evolutionary_operators.planEvaluatorSetup.preparePlanEvaluator(loadedData)

    plan = population[indivNr]
    #C++ return by reference below.
    print "Exporting this plan: ", plan
    #I have to supply two dummy-arguments due to the way SWIG converts c++ pass-by-reference to Python.
    print "Interpreting plan ", plan
    [plannedPositions, plannedOrientations] = evaluation_function.interpretAndExportPlan(plan, [], [])
    #Tuple to array
    plannedPositions=list(plannedPositions)
    plannedOrientations=list(plannedOrientations)
    if min_turn_angle:
        plannedPositions, plannedOrientations = split_sharply_turning_waypoints(plannedPositions, plannedOrientations, min_turn_angle)

    print "Resulting positions were ", plannedPositions
    print "PlannedOrientations is ", plannedOrientations


    # TODO: Am I correct in rotating before translating here?
    if rotationDegreesAroundzAxis:
        plannedPositions = generate_rotated_vectors_around_z(plannedPositions,rotationDegreesAroundzAxis)
        plannedPositions = [p.tolist() for p in plannedPositions] # Method returns list of np-arrays. Converting to list of lists.
        #TODO: I think I need to rotate the positions and orientations equally here.
        plannedOrientations = generate_rotated_vectors_around_z(plannedOrientations,rotationDegreesAroundzAxis)
        plannedOrientations = [p.tolist() for p in plannedOrientations]
    print "Planned orientations were ", plannedOrientations
    # The returned orientation is an orientation vector (x,y,z). I need to convert this to (roll,pitch,yaw) before exporting.
    # For the inspection missions, we want to keep roll and pitch to zero, so I'm only exporting the yaw.
    plannedOrientations = generate_simplified_roll_pitch_yaw(plannedOrientations)
    print "Planned orientations as rpy were ", plannedOrientations

    if positionOffsets!=None:
        plannedPositions=addOffsetToEachSublist(plannedPositions,positionOffsets)

    #Converting the data to the format we want to write to the yaml-file.
    #offset = convert_pair_of_lists_to_dict(['x','y','z'],plannedPositions[0])
    #Not using the "offset" anymore. The offset is baked into the first waypoint. However, notice that this waypoint does not have a sensible orientation,
    #since the path to that waypoint is not part of the plan.

    #Dummy heading to follow to initial waypoint
    plannedOrientations.insert(0, [0,0,0])

    waypoints = []
    #TODO: This works as long as we have one more waypoint than edge. Consider how to handle cases where starting point is pre-determined...
    for planEdgeNr in range(0,len(plannedOrientations)):
        currentOrientation = list(plannedOrientations[planEdgeNr])
        currentWaypoint = plannedPositions[planEdgeNr]


        wpHash = convert_pair_of_lists_to_dict(['x','y','z'],currentWaypoint)
        orientationHash = convert_pair_of_lists_to_dict(['r','p','y'],currentOrientation)
        wpAndOrientation = {"position":wpHash, "orientation":orientationHash}
        waypoints.append(wpAndOrientation)

    yamlDict = {'waypoints': waypoints}

    outputFile = open(yamlPath, 'w')
    print "Dumping: ", yamlDict
    yaml.dump(yamlDict, outputFile, default_flow_style=False, allow_unicode=True)

def exportAllWinnerIndividuals(results_root_folder, positionOffsets = None, rotationDegreesAroundzAxis = 0, min_turn_angle = None):
    print "Pos offsets was ", positionOffsets
    print "rot degrees was ", rotationDegreesAroundzAxis
    population_folder = os.path.join(results_root_folder,
                                     Constants_and_Datastructures.populationsSubFolder)
    winners_file_path = os.path.join(population_folder,
                                     Constants_and_Datastructures.FINAL_POPULATION_FILE_PKL)
    yml_store_folder = os.path.join(results_root_folder, Constants_and_Datastructures.ymlSubfolder)


    # Generating store folder if it doesn't exist
    if not os.path.exists(yml_store_folder):
        os.makedirs(yml_store_folder)
    with open(winners_file_path,"rb") as f:
        loadedData = pickle.load(f)
    populationSize = len(loadedData[Constants_and_Datastructures.populationName])
    evaluation_function = evolutionary_operators.planEvaluatorSetup.preparePlanEvaluator(loadedData)
    for i in range(populationSize):
        exportPlanToYaml(winners_file_path, os.path.join(yml_store_folder,"plan_"+str(i)+".yml"), i, positionOffsets=positionOffsets,
                         rotationDegreesAroundzAxis=rotationDegreesAroundzAxis, evaluation_function=evaluation_function,
                         min_turn_angle=min_turn_angle)

def parse_input_arguments():
    # Parsing user inputs:
    parser = argparse.ArgumentParser(
        description='Allows the user to export generated plans as yml-files, which can later be imported in Rock.\n'
                    'The user supplies the root folder of his results, plans are automatically extracted,\n'
                    'stored in a subfolder. An optional argument also allows the plans to be offset and/or rotated.\n')

    # Mandatory Arguments
    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('-i','--input_folder', type=str, required=True,
                                     help='Path to folder containing your plans (this should be your results-folder,'
                                          ' which contains the sub-folder "populations").')

    #Optional arguments
    parser.add_argument('--position_offset', type=float, nargs=3, required=False,
                        help='If given, this serves as an offset to the position of each point in the plan. Given as 3 numbers separated by space, '
                             'representing the offset in x,y and z.')

    parser.add_argument('--rotation_offset', type=float, required=False,
                        help='If given, this serves as an offset (in radians) of the plan, including the robot orientation.'
                             ' This could be useful if the orientation of the 3D model we planned for does not match the '
                             'orientation of the real inspection target.')


    parser.add_argument('--min_turn_angle', type=float, required=False,
                    help='If given, this splits any too sharp turns in the plan into two less sharp turns. This can be'
                         ' useful when we require smoother motion, for instance nice for 3D reconstruction. The given '
                         'angle is in radians, and any turn sharper than this (in other words, less radians), will '
                         'be split.')

    args = parser.parse_args()
    return args

if __name__ == "__main__":

    args = parse_input_arguments()
    rotation_degrees = 0
    if args.rotation_offset:
        rotation_degrees = math.degrees(args.rotation_offset)
    exportAllWinnerIndividuals(args.input_folder,positionOffsets=args.position_offset,rotationDegreesAroundzAxis=rotation_degrees, min_turn_angle=args.min_turn_angle)
    #Example for gazebo: --position_offset 35 -10 -35 --rotation_offset 0.7854

    #TODO: Now, from where can I get those offsets? Discuss with Gustavo.
    #loadFile = "/home/kaiolae/PycharmProjects/deap_framework/results/2015-09-01-downcam-True/run0/winner_indivs.pkl"
    #indivNr = 8

    #exportedYamlPath = "exported_plans/pos_angle_offset.yml"
    # TODO: I think orientation offsets are not the right way to handle rotation of the model. I need to transform all waypoints with a rotation around center instead.
    #exportPlanToYaml(loadFile,exportedYamlPath,indivNr,positionOffsets=(35,-10,-15), rotationDegreesAroundzAxis=math.degrees(0.7854))