import pickle

import math

import settings

__author__ = 'kaiolae'
import sys
import argparse
import os
import imp
import shutil
import Constants_and_Datastructures
import numpy as np
import yaml
import settings.runtime_specified_parameters as runtime_specified_parameters

# Uses argparse to parse arguments, and returns the args object.
def parse_inspection_planning_arguments():
    # Parsing user inputs:
    parser = argparse.ArgumentParser(
        description='Generates a number of inspection plans for a 3D structure given as input.\n'
                    'The inspection plans represent the optimal ways to balance energy usage and inspection coverage,\n'
                    'ranging from short and effective to long and thorough plans.\n'
                    'Generated plans are stored to the destination folder given as input argument.')

    # Mandatory Arguments for all plan types
    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('-i','--input_model_path', type=str, required=True,
                                     help='Path to the 3D model we want to plan for.')
    mandatory_arguments.add_argument('-o','--output_folder', type=str, required=True,
                                     help='path to the folder where we want to store planning results.')
    parser.add_argument('--disable_seeding', action='store_true', help='If given, seeding plans are not used in optimization. Using seeds usually works better,'
                                                                       ' but disabling them can be useful for comparisons.')
    parser.add_argument('--multirun', action='store_true',
                        help='Enables a more sensible storing scheme for experiments where we run the algorithm multiple times (for instance to gather statistics)')
    parser.add_argument('--no_post_processing', action='store_true',
                        help='Only optimize and dump pickled results. Post-processing can be done on the pickled results later.')
    parser.add_argument('--set_structure_viewpoint', action='store_true',
                        help='If given, the user can define a viewpoint for this structure, from which images of plans will be shown.')
    # TODO: Make sure the input arguments are somehow stored together with the other data from the run. Maybe as a separate file?

    args = parser.parse_args()

    # Checking input arguments - in a multirun, these are checked earlier, to avoid redoing this for each run.
    if not args.multirun:
        check_existence_and_make(args.output_folder)

    return args

def parse_circling_planning_arguments():
    # Parsing user inputs:
    parser = argparse.ArgumentParser(
        description='Generates a number of inspection plans for a 3D structure given as input.\n'
                    'The inspection plans circle around the structure different number of times,\n'
                    'ranging from short and effective to long and thorough plans.\n'
                    'Generated plans are stored to the destination folder given as input argument.')
    # Mandatory Arguments for all plan types
    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('-i','--input_model_path', type=str, required=True,
                                     help='Path to the 3D model we want to plan for.')
    mandatory_arguments.add_argument('-o','--output_folder', type=str, required=True,
                                     help='path to the folder where we want to store planning results.')

    #Optional arguments
    parser.add_argument('--set_structure_viewpoint', action='store_true',
                        help='If given, the user can define a viewpoint for this structure, from which images of plans will be shown.')

    return parser.parse_args()

def check_existence_and_make(folder):
    if not os.path.exists(folder):
        os.makedirs(folder)
    else:
        do_overwrite = query_yes_no("The storage folder " + folder + " already exists. Continuing means previously stored data in"
                                    " that folder may be overwritten. Are you sure you want to continue?", default=None)
        if not do_overwrite:
            # If the user does not want to overwrite his data, we exit the program so he can enter another storage folder.
            sys.exit(0)

# Stores info about the current run, to help us analyze and reproduce it.
# StorageFolder: Where to store the info
# ParamterFile: A python file containing storage paths that we need.
def store_run_info(storageFolder):
    experiment_settings_folder = os.path.join(storageFolder, Constants_and_Datastructures.parametersSubFolder)
    if not os.path.exists(experiment_settings_folder):
        os.makedirs(experiment_settings_folder)
    shutil.copy(Constants_and_Datastructures.SOURCE_CODE_ROOT+"/settings/Parameters.py", experiment_settings_folder)
    shutil.copy(Constants_and_Datastructures.PATH_TO_CONSTANTS_FILE, experiment_settings_folder)


def query_yes_no(question, default="yes"):
    """Ask a yes/no question via raw_input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
        It must be "yes" (the default), "no" or None (meaning
        an answer is required of the user).

    The "answer" return value is True for "yes" or False for "no".
    """
    valid = {"yes": True, "y": True, "ye": True,
             "no": False, "n": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "
                             "(or 'y' or 'n').\n")


def importFromURI(uri, absl=False):
    if not absl:
        uri = os.path.normpath(os.path.join(os.path.dirname(__file__), uri))
    path, fname = os.path.split(uri)
    mname, ext = os.path.splitext(fname)

    no_ext = os.path.join(path, mname)

    print
    if os.path.exists(no_ext + '.py'):
        try:
            print "Loading module: ", mname
            return imp.load_source(mname, no_ext + '.py')
        except:
            print "Error: Could not import this python module: ", mname
            raise
    else:
        raise IOError("Error: The python module you tried to load does not exist on the filesystem: ", no_ext)


# Returns all files of type filetype from folder.
def select_files_of_type(folder,file_ending):
    return [os.path.join(folder,f) for f in os.listdir(folder) if (os.path.isfile(os.path.join(folder,f)) and file_ending in f)]


def getFilesRecursively(folder, fileName):
    #Returns all the files named "fileName" somewhere under folder.

    foundFiles = []

    for root, dirs, files in os.walk(folder, topdown=False):
        for name in files:
            if fileName in os.path.basename(name): #Using the "in-operator", to allow searching e.g. for ALL files containing .pkl.
                foundFiles.append(root+"/"+name)

    return foundFiles

def getSubFoldersRecursively(folder, subFolderName):
    #Returns all the folders named "subFolderName" somewhere under folder.

    foundFolders = []

    for root, dirs, files in os.walk(folder, topdown=False):
        for name in dirs:
            if subFolderName in os.path.basename(name): #Using the "in-operator", to allow searching e.g. for ALL files containing .pkl.
                foundFolders.append(root+"/"+name)

    return foundFolders

def load_dumped_population(file_path):

    if "pkl" in file_path:
        return pickle.load(open(file_path, "rb"))
    elif "yml" in file_path:
        return yaml.load(open(file_path, "rb"))

    raise ValueError("Loaded population file is of an unsupported file type: ", file_path)


def population_object_to_list(pop_obj):
    """
    Converts a population object to a list, where each entry in the list is an individual,
    with all properties as dictionary entries. Useful for storage purposes.
    """
    population_list = []
    for ind in pop_obj:
        i = {}
        # Each ind is an object. We have to extract all its relevant values and store them. That is the price of not pickling.
        i["fitness"] = ind.fitness.values
        i["fitness_weights"] = ind.fitness.weights
        i["genotype1"] = list(ind)
        #Set genotype 2 if the object has one.
        try:
            i["genotype2"] = ind.genotype2
        except AttributeError:
            i["genotype2"] = None
        i["type"] = str(type(ind))
        population_list.append(i)

    return population_list




def unit_vector(vector):
    """ Returns the unit vector of the vector.  """
    return vector / np.linalg.norm(vector)

def angle_between(v1, v2):
    """ Returns the angle in radians between vectors 'v1' and 'v2'::

            angle_between((1, 0, 0), (0, 1, 0))
                1.5707963267948966
            angle_between((1, 0, 0), (1, 0, 0))
                0.0
            angle_between((1, 0, 0), (-1, 0, 0))
                3.141592653589793
    """
    v1_u = unit_vector(v1)
    v2_u = unit_vector(v2)
    angle = np.arccos(np.dot(v1_u, v2_u))
    if np.isnan(angle):
        if (v1_u == v2_u).all():
            return 0.0
        else:
            return np.pi
    return angle

def rotation_matrix(axis, theta):
    """
    Returns the rotation matrix associated with counterclockwise rotation about
    the given axis by theta radians.
    :param axis: The axis to rotate about
    :param theta: The number of radians to rotate
    :return: A rotation matrix for the requested rotation.
    """
    axis = np.asarray(axis)
    theta = np.asarray(theta)
    axis = axis / math.sqrt(np.dot(axis, axis))
    a = math.cos(theta / 2)
    b, c, d = -axis * math.sin(theta / 2)
    aa, bb, cc, dd = a * a, b * b, c * c, d * d
    bc, ad, ac, ab, bd, cd = b * c, a * d, a * c, a * b, b * d, c * d
    return np.array([[aa + bb - cc - dd, 2 * (bc + ad), 2 * (bd - ac)],
                     [2 * (bc - ad), aa + cc - bb - dd, 2 * (cd + ab)],
                     [2 * (bd + ac), 2 * (cd - ab), aa + dd - bb - cc]])


def generate_rotated_vectors_around_z(vectors, rotation_degrees):
    """
    Rotates each list of vectors in the input by rotation_degrees around the z-axis, and returns a list
    of the resulting rotated vectors.
    :param vectors: A list of vectors we want to rotate.
    :param rotation_degrees: The amount we want to rotate each vector by
    :return: List of rotated vectors.
    """
    rotated_vectors = []
    for v in vectors:
        up_vector = np.array([0,0,1])
        dotprod = np.dot(rotation_matrix(up_vector, math.radians(rotation_degrees)), v)
        rotated_vectors.append(np.array(dotprod))
    return rotated_vectors

def generate_simplified_roll_pitch_yaw(viewing_vectors):
    """
    Generates a sequence of orientations in roll, pitch, yaw format based on
    an input sequence of orientation vectors. This simplified algorithm, however,
    always sets roll and pitch to 0, and is suitable in situations where we only want orientations
    to change by changing the yaw.
    :param viewing_vectors: A sequence of orientation vectors.
    :return: A sequence [roll, pitch, yaw] triplets, where roll and pitch are always 0
    in this simplified version.
    """
    rpy_values = []
    for v in viewing_vectors:
        #yaw = math.atan2(v.y, v.x)
        yaw = math.atan2(v[1], v[0])
        rpy_values.append([0, 0, yaw])

    print "generated rpy vals: ", rpy_values
    return rpy_values

#Stores individuals, as well as parameters on how they were fitness tested.
def storePopulationAndParameters(indivs, storageFileName, outputFolder, structure_path,
                                 parameters_file, paretoFront = None, elapsedTime = None):
    mainFolder = outputFolder
    popSubFolder = os.path.join(mainFolder, settings.Constants_and_Datastructures.populationsSubFolder)
    if not os.path.exists(mainFolder):
        # Generating the main folder for the experiment
        os.makedirs(outputFolder)
    if not os.path.exists(popSubFolder):
        os.makedirs(popSubFolder)
    storageDict = {settings.Constants_and_Datastructures.populationName : indivs, settings.Constants_and_Datastructures.sceneName : structure_path, settings.Constants_and_Datastructures.originName : parameters_file.PLAN_ORIGIN,
                   settings.Constants_and_Datastructures.sensorParamsName : parameters_file.SENSOR_PARAMETERS, settings.Constants_and_Datastructures.paretoFrontName : paretoFront,
                   settings.Constants_and_Datastructures.maxEnergyName: runtime_specified_parameters.max_energy_usage, settings.Constants_and_Datastructures.numMemoizedSolutionsName: runtime_specified_parameters.num_memoized_edges,
                   settings.Constants_and_Datastructures.elapsedTimeName : elapsedTime, settings.Constants_and_Datastructures.planLoopsAroundName : parameters_file.PLAN_LOOPS_AROUND,
                   }
    with open(os.path.join(popSubFolder,storageFileName) , "wb") as store_file:
        if ".pkl" in storageFileName:
            pickle.dump(storageDict,store_file) #I can also store many other things, such as generation numbers, run stats, etc. See http://deap.readthedocs.org/en/master/tutorials/advanced/checkpoint.html
        elif ".yml" in storageFileName or ".yaml" in store_file:
            # We cannot store an object in a yml-file (as we can in the pickle file), so this conversion is necessarry.
            storageDict[settings.Constants_and_Datastructures.populationName] = population_object_to_list(indivs)
            if paretoFront:
                storageDict[settings.Constants_and_Datastructures.paretoFrontName] = population_object_to_list(paretoFront)
            yaml.dump(storageDict, store_file, default_flow_style=False, allow_unicode=True)
            print "yml dumped to ", store_file
        else:
            raise TypeError("Trying to store results in file of unsupported type.")