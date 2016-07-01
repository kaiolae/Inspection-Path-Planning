# Iterates over different degrees of "complete coverage", in the complete coverage plan generator by Englot and Hover.
# Coverage and energy data are returned from the planner, and stored by this script. The planner itself stores plan pictures.
import argparse
import copy
import os
import subprocess

import csv

from collections import namedtuple
import itertools
import numpy as np
import time
import yaml

import settings.Utilities as Utilities
import settings.Parameters as params
import settings.Constants_and_Datastructures as constants

IMAGE_SUBFOLDER = "images"
PLAN_SUBFOLDER = "plans"

#These are some not-so-robust ways to get data back from the c-code running this. TODO: Store and load data instead.
ENERGY_RESULT_LINE = "Energy result:"
COVERAGE_RESULT_LINE = "Coverage result:"
NODE_COUNT_LINE = "Total node count:"

def iterate_over_coverage_degrees_and_plan(inspection_target, collision_target, storage_folder, min_cov, max_cov, num_samples, start_pos=None, num_repetitions = 1,
                                           suppress_loopback = False, no_image_storing = False):
    print storage_folder
    score_file = os.path.join(storage_folder,"scores.yml")
    plans_folder_path = os.path.join(storage_folder,PLAN_SUBFOLDER)
    images_folder_path = os.path.join(storage_folder,IMAGE_SUBFOLDER)
    if not os.path.exists(plans_folder_path):
        os.makedirs(plans_folder_path)
    if not os.path.exists(images_folder_path):
        os.makedirs(images_folder_path)
    print plans_folder_path
    #Utilities.check_existence_and_make(plans_folder_path)
    #Utilities.check_existence_and_make(images_folder_path)

    target_name = os.path.basename(inspection_target)
    common_args = [constants.PATH_TO_COMPLETE_COVERAGE_RUNNABLE, "--iTarget", inspection_target, "--cTarget", collision_target, "--optimizationType", 'l',
            "--camSpecs", [str(sp) for sp in params.SENSOR_PARAMETERS]]
    if start_pos:
        common_args.extend(["--startPosition", [str(s) for s in start_pos]])

    if suppress_loopback:
        common_args.append("--suppress_loopback")

    if no_image_storing:
        common_args.append("--silent")

    #Flattening the list of args.
    flattened_args = []
    for elem in common_args:
        if not isinstance(elem, basestring):
            for subelem in elem:
                flattened_args.append(subelem)
        else:
            flattened_args.append(elem)

    print "flattened: ", flattened_args
    #Adding the camera parameters.
    #for camParam in params.SENSOR_PARAMETERS:
    #    common_args.append(str(camParam))


    # We need to calculate the highest-covering plans first. This is because we will use
    # the number of nodes in the maximally-covering plan as a target for lower-covering plan sampling, ensuring we
    # sample sufficiently.

    insert_csv_header(score_file)

    # TODO: Note I am turning off this max nodes stuff. It is so complicated to explain, and does not help very much.
    res = run_complete_coverage(max_cov, flattened_args, images_folder_path, plans_folder_path, score_file, num_repetitions)
    #max_nodes = 0
     #for r in res:
    #    if r.Nodes > max_nodes:
    #        max_nodes = r.Nodes
    #print "Max nodes was ", max_nodes
    #flattened_args.extend(["--minNumWaypointSamples", str(int(max_nodes))])

    for cov_degree in np.linspace(min_cov, max_cov, num_samples, endpoint=False):
        run_complete_coverage(cov_degree, flattened_args, images_folder_path, plans_folder_path, score_file, num_repetitions)


def run_complete_coverage(cov_degree, common_function_arguments, images_folder_path, plans_folder_path, append_to_file_path, num_repetitions = 1):

    results = []

    for i in range(num_repetitions):
        picture_filepath = os.path.join(images_folder_path, "coverage_" + str(cov_degree) + "_plan"+str(i)+"step1.png")
        plan_filepath = os.path.join(plans_folder_path, "coverage_" + str(cov_degree) + "_plan"+str(i)+ ".dat")
        specific_args = ["--storageFile", plan_filepath,
                         "--imageStore", picture_filepath, "--coverageDegree", str(cov_degree)]
        total_args = copy.deepcopy(common_function_arguments)
        total_args.extend(specific_args)
        print "Args are ", " ".join(map(str, total_args))
        start = time.time()
        popen = subprocess.Popen(total_args, stdout=subprocess.PIPE)
        lines_iterator = iter(popen.stdout.readline, b"")  # To print c++ output as it arrives
        energy_result = -1
        coverage_result = -1
        number_of_nodes_total = -1  # TODO: This should be used to set the value MIN_NUM_SAMPLED_CONFIGURATIONS in C++ code.
        for line in lines_iterator:
            #TODO: To avoid noisy printing, just remove.
            #if "Sampled starting position:" in line:
            print line
            if ENERGY_RESULT_LINE in line:
                energy_result = float(line.rsplit(None, 1)[-1])
            if COVERAGE_RESULT_LINE in line:
                coverage_result = float(line.rsplit(None, 1)[-1])
            if NODE_COUNT_LINE in line:
                number_of_nodes_total = float(line.rsplit(None, 1)[-1])

        elapsed = time.time() - start
        result = constants.ResultsStructure(Coverage= coverage_result, Energy=energy_result, Nodes= number_of_nodes_total, TargetCoverage=cov_degree, TimeSpent=elapsed)

        append_result_to_csv(append_to_file_path, coverage_result, energy_result, number_of_nodes_total, cov_degree, elapsed)

        results.append(result)

    return results

def append_result_to_csv(filepath, coverage_result, energy_result, number_of_nodes, input_cov_degree, elapsed_time):
    outputFile = open(filepath, 'ab')
    wr = csv.writer(outputFile, delimiter='\t')
    wr.writerow([input_cov_degree, coverage_result, energy_result, number_of_nodes, elapsed_time])
    outputFile.close()

def insert_csv_header(filepath):
    outputFile = open(filepath, 'ab')
    wr = csv.writer(outputFile, delimiter='\t')
    wr.writerow(["Input_Coverage_Degree", "Coverage", "Energy", "Number_of_Nodes", "Time_Elapsed"])
    outputFile.close()

def parseArguments():
    parser = argparse.ArgumentParser(
    description='# Iterates over different degrees of "complete coverage", in the complete coverage plan generator by Englot and Hover.'
                'Coverage and energy data are returned from the planner, and stored by this script. The planner itself stores plan pictures.')


    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('-i', '--inspection_target_filepath', type=str, required=True)
    mandatory_arguments.add_argument('-c', '--collision_target_filepath', type=str, required=True)
    mandatory_arguments.add_argument('-s', '--storage_folder', type=str, required=True)
    mandatory_arguments.add_argument('-m', '--minimun_coverage_degree', type=float, required=True)
    mandatory_arguments.add_argument('-a', '--maximum_coverage_degree', type=float, required=True)
    mandatory_arguments.add_argument('-n', '--num_samples', type=int, required=True,
                                     help="The number of equally spaced samples between minimum - and maximum_coverage_degree which we want to generate plans for.")

    #Optional arguments
    #If no start position is given, it is sampled randomly for each run.
    mandatory_arguments.add_argument('-p', '--start_pos', type=float, nargs=3, required=False)  # Decent start pos for manifold: 10,10,5
    #Optional arguments
    parser.add_argument('-r', '--num_repetitions', default=1, type=int,
                        help='(optional) The number of individual repititions of the coverage calculations. Important for '
                             'statistics-gathering runs.')
    parser.add_argument('--suppress_loopback', action = 'store_true', help="If this flag is set, the complete coverage plans are "
                                                                           "no longer required to loop back to their starting position."
                                                                           "Only has effect for complete plans (that is, all sub-loops connected).")
    parser.add_argument("--no_image_storing", action = "store_true", help="To disable storing an image of each plan")


    args = parser.parse_args()
    Utilities.check_existence_and_make(args.storage_folder)
    return args


#NOTE: The behavior is to append new results to any existing results - allowing us to continue aborted experiments. If a fresh start is needed,
#delete the previous files or choose a different output folder.
if __name__ == "__main__":
    args = parseArguments()
    iterate_over_coverage_degrees_and_plan(args.inspection_target_filepath, args.collision_target_filepath,
                                           args.storage_folder, args.minimun_coverage_degree, args.maximum_coverage_degree,
                                           args.num_samples, args.start_pos, args.num_repetitions, args.suppress_loopback,
                                           args.no_image_storing)