#Many methods for plotting data from optimization runs. Not necessarry if you just want to generate plans,
#but may come in handy if you want to do some performance analyses.

import os
import pickle
import re
from collections import namedtuple
from os import listdir
from os.path import isfile, join, isdir

import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt
import numpy as np
import operator
import scikits.bootstrap as bs
import scipy.stats as st
import yaml

from settings import Utilities
from settings.Parameters import HYPERVOLUME_REF_POINT # BoxIndividual, MovementActionIndividual, Fitness,
import settings.Constants_and_Datastructures
import plan_post_processing.attainmentSurfaceAnalysis

#This Python package can be downloaded from https://ls11-www.cs.uni-dortmund.de/_media/rudolph/hypervolume/hv_python.zip
from hv import HyperVolume
from settings.Utilities import getFilesRecursively

__author__ = 'kaiolae'

default_colors = ["blue", "green", "red", "cyan", "magenta", "black"]
#colors = ["black", "black","black", "black","black", "black"] #For BW plots

default_markers = ["None","o", "^"]
#Changing the font info for the entire plot
font = {'family' : 'normal',
        'weight' : 'bold',
        'size'   : 30}

legend_font_size = 25
p_val_font_size = 20
ticks_font_size = 23

plt.rcParams['xtick.labelsize'] = ticks_font_size
plt.rcParams['ytick.labelsize'] = ticks_font_size
plt.rc('font', **font)

# The following three lines make sure fonts are Type1 rather than Type3.

#plt.rcParams['ps.useafm'] = True
#plt.rcParams['pdf.use14corefonts'] = True
#plt.rcParams['text.usetex'] = True

plt.rc('font', **{'family': 'serif', 'serif': ['Helvetica']})
plt.rc('text', usetex=True)


def tex_escape(text):
    """
        :param text: a plain text message
        :return: the message escaped to appear correctly in LaTeX
    """
    conv = {
        '&': r'\&',
        '%': r'\%',
        '$': r'\$',
        '#': r'\#',
        '_': r'\_',
        '{': r'\{',
        '}': r'\}',
        '~': r'\textasciitilde{}',
        '^': r'\^{}',
        '\\': r'\textbackslash{}',
        '<': r'\textless',
        '>': r'\textgreater',
    }
    regex = re.compile('|'.join(re.escape(unicode(key)) for key in sorted(conv.keys(), key = lambda item: - len(item))))
    return regex.sub(lambda match: conv[match.group()], text)

def plotFitness(logbook, block=True):

    plt.close()

    gen = logbook.select("gen")
    fit_max = logbook.chapters["fitness"].select("max")
    fit_avg = logbook.chapters["fitness"].select("avg")
    fit_min = logbook.chapters["fitness"].select("min")

    print fit_max
    print "Fit max 0 is ", [f[0] for f in fit_max]


    plt.clf()
    fig, ax1 = plt.subplots()

    ax1.plot(gen, [f[0] for f in fit_min], "b-", label="Minimum Fitness")
    ax1.plot(gen, [f[0] for f in fit_avg], "b--", label="Average Fitness")

    ax1.set_xlabel("Generation")
    ax1.set_ylabel("Fitness", color="b")
    for tl in ax1.get_yticklabels():
        tl.set_color("b")

    ax2 = ax1.twinx()
    ax2.plot(gen, [f[1] for f in fit_avg], "g--", label="Average Length")
    ax2.plot(gen, [f[1] for f in fit_max], "g-", label="Max Length")
    ax2.plot(gen, [f[1] for f in fit_min], "g.-", label="Min Length")
    ax2.set_ylabel("Energy", color="g")
    for tl in ax2.get_yticklabels():
        tl.set_color("g")

    handles, labels = ax1.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()


    handles.extend(handles2)
    labels.extend(labels2)

    ax1.legend(handles, labels, loc="center right")
    plt.show(block=block)


def plotParetoFront2D(obj1_values, obj2_values, color="black", marker="o", label=None, obj1_name = "Coverage Score", obj2_name = "Energy Used", z_order=1, alpha=1, scatter_size=40):
    if label:
        plt.scatter(obj1_values, obj2_values,color=color, marker=marker, s=scatter_size, label = label, zorder=z_order, alpha=alpha) #s sets the size
    else:
        plt.scatter(obj1_values, obj2_values, s=scatter_size, color = color, marker=marker, zorder=z_order, alpha=alpha)
    plt.xlabel(obj1_name)
    plt.ylabel(obj2_name)


#Given all individuals on the pareto front, plots them according to their 2 first objectives.
def individuals_to_pareto_front_2D(individuals_file,color="black", marker="o", label=None, z_order=1, alpha=1,  scatter_size=40):
    data = Utilities.load_dumped_population(individuals_file)#pickle.load(open(individuals_file, "rb"))


    individuals = data[settings.Constants_and_Datastructures.populationName]
    try:
        obj1 = [i.fitness.values[0] for i in individuals]
        obj2 = [i.fitness.values[1] for i in individuals]
    except AttributeError:  # If we have a dict instead of an object here, we extract it this way.
        obj1 = [i['fitness'][0] for i in individuals]
        obj2 = [i['fitness'][1] for i in individuals]

    for cov, eng in zip(obj1,obj2):
        if cov == 0:
            print "Complete coverage score: " + str(eng)

    plotParetoFront2D(obj1, obj2, color, marker, label, z_order=z_order, alpha=alpha, scatter_size=scatter_size)

    #Returning the max energy.
    return min(obj1), max(obj2)


ResultsStructure = namedtuple("ResultsStructure", "Coverage Energy Nodes")

def results_to_pareto_front_2D(results_file, color="black", marker="o", label=None):
    '''
    Assumed the input list of dicts contains exactly 2 objectives.
    :param results_file: File containing list of results, where each element has a score along both objectives.
    :param color:
    :param marker:
    :param label:
    :return:
    '''
    list_of_results = yaml.load(open(results_file,"rb"))
    print list_of_results
    obj1_values = [r.Energy for r in list_of_results]
    obj2_values = [r.Coverage for r in list_of_results]
    # Swapping here to plot coverage on x-axis. TODO: Maybe make that an input argument?
    plotParetoFront2D(obj2_values, obj1_values, color, marker, label, obj1_name="Energy", obj2_name="Coverage")

def csv_results_to_pareto_front_2D(results_file, x_data = 'Energy', y_data = 'Coverage', color="black", marker="o", label=None, z_order=1, alpha=1, scatter_size=40):
   results = np.genfromtxt(results_file, names=True) #Practical way of reading headers and columns into dict-like structure.
   plotParetoFront2D(results[x_data], results[y_data], color, marker, label, obj1_name=y_data, obj2_name=x_data, z_order=z_order, alpha=alpha,scatter_size=scatter_size)
   return min(results[x_data]), max(results[y_data]) #Returning the max energy value and min coverage value.

def csv_results_to_statistics(results_file, field_name):
    results = np.genfromtxt(results_file, names=True)
    valueArray = np.array(results[field_name])
    median = np.median(valueArray)
    CIs = bs.ci(valueArray, statfunction=np.median, n_samples=5000)

    return median, CIs[0], CIs[1]


def plot_coverage_degree_vs_time(cc_scores_file):

    boxwidth = 0.05

    results = np.genfromtxt(cc_scores_file, names=True)
    coverage_and_time = {}
    for r in results:
        cov = float(r["Input_Coverage_Degree"])
        time = r["Time_Elapsed"]
        if not cov in coverage_and_time.keys():
            coverage_and_time[cov] = []
        coverage_and_time[cov].append(time)

    # Sort by input coverage degree
    sorted_dict = sorted(coverage_and_time.items(), key=operator.itemgetter(0))
    print sorted_dict

    #TODO: reduce number of numbers after decimal
    plt.boxplot([item[1] for item in sorted_dict], positions = [item[0] for item in sorted_dict], widths = boxwidth,
                manage_xticks=False)

    max_x = max(coverage_and_time.keys())
    min_x = min(coverage_and_time.keys())
    plt.xlim([min_x - boxwidth, max_x + boxwidth])
    plt.ylabel("Plan Generation Time (s)")
    plt.xlabel("Required Coverage Degree")
    plt.tight_layout()
    #plt.xticks(np.arange(min_x, max_x, 0.1))

    #for cov, time in coverage_and_time.items():
    #    print time
    #    plt.boxplot(np.array(time))


    #plt.show()



def generateLabel(folderName):
    # Turns a folder name into a short and nice label name for our plot.
    print "Generating Label"
    if "False-during-evolution-true-during-testing" in folderName:
        return "Opt: Single-Cam. Test: Dual-Cam."
    if "without-downcam" in folderName or "downcam-False" in folderName:
        return "Opt: Single-Cam. Test: Single-Cam."
    if "with-downcam" in folderName or "downcam-True" in folderName:
        return "Opt: Dual-Cam.  Test: Dual-Cam."
    print "WARNING: No label assigned for this folder: ", folderName
    return tex_escape(folderName) #to avoid latex-problems

def plotFeatureOverGenerations(folderPath,feature, label=None):
    #Allows the plotting of any of a number of features over the generations of evolution.
    files = [ f for f in listdir(folderPath) if isfile(join(folderPath,f)) ]

    y_values = []
    y_values_names = []
    gens = []
    for f in files:
        if "gen" in f and ".pkl" in f:
            genNr = int(f.split("gen")[1].split(".")[0])
            gens.append(genNr)

            fullName = join(folderPath,f)
            data = pickle.load(open(fullName,"rb"))
            population = data[settings.Constants_and_Datastructures.populationName]

            if feature == settings.Constants_and_Datastructures.lengthName:
                if len(y_values) == 0:
                    y_values=[[],[],[]]
                    y_values_names = ["Average Length", "Max Length", "Standard Deviation"]

                populationLengths = []
                for p in population:
                    populationLengths.append(len(p))

                y_values[0].append(float(sum(populationLengths))/float(len(populationLengths)))
                y_values[1].append(max(populationLengths))
                y_values[2].append(np.std(np.array(populationLengths)))

            if feature == settings.Constants_and_Datastructures.hyperVolumeName:
                if len(y_values) == 0:
                    y_values=[[]]
                    y_values_names = ["Hypervolume"]
                hyperVolumeReference = HYPERVOLUME_REF_POINT
                if settings.Constants_and_Datastructures.maxEnergyName in data.keys():
                    hyperVolumeReference[1] = data[settings.Constants_and_Datastructures.maxEnergyName] * 1.01
                    print "New hypervolume is ", hyperVolumeReference
                hypVolCalculator = HyperVolume(hyperVolumeReference)
                front = [i.fitness.values for i in population]
                hypVol = hypVolCalculator.compute(front)
                y_values[0].append(hypVol)

            if feature == settings.Constants_and_Datastructures.numMemoizedSolutionsName:
                if len(y_values) == 0:
                    y_values=[[]]
                    y_values_names = ["Elements Memoized"]
                y_values[0].append(data[settings.Constants_and_Datastructures.numMemoizedSolutionsName])

    #Code to sort the plotted data
    gensArray = np.array(gens)

    index_to_order_by = gensArray.argsort()
    gensOrdered = gensArray[index_to_order_by]

    for valueSet, name in zip(y_values, y_values_names):
        y_values_array = np.array(valueSet)
        y_values_ordered = y_values_array[index_to_order_by]
        if not label:
            label = generateLabel(name)
        plt.plot(gensOrdered,y_values_ordered, marker = "o", label=label)


    folderName = os.path.split(folderPath[:-1])[1]
    folderNameEscaped = tex_escape(folderName)
    plt.title("Data for " + folderNameEscaped) #Gives the name of the folder
    plt.legend()
    #plt.show()

#Assumes two time series gathered from two different expermimental treatment.
#At each timestep, the two treatments are compared, and p-values calculated and plotted.
def plotPValues(treatment1, treatment2, plotTo):
    p_threshold = 0.01
    #Setting up the plot
    plotTo.get_yaxis().set_ticks([])
    plotTo.set_ylim(0, 2)
    plotTo.set_xlim(0,len(treatment1[0])-1)
    plotTo.set_ylabel("p$<$"+str(p_threshold),fontsize=p_val_font_size)
    plotTo.get_yaxis().set_ticks([])
    plotTo.get_xaxis().set_ticklabels([])
    #Calculating plotted values
    for genNr in range(len(treatment1[0])):
        print genNr
        treatment1Values = [t1[genNr] for t1 in treatment1]
        treatment2Values = [t2[genNr] for t2 in treatment2]
        u_value, p_value = st.mannwhitneyu(treatment1Values,treatment2Values)
        print "medians were ", np.median(treatment1Values), " and ", np.median(treatment2Values)
        print "lenghts were ", len(treatment1Values), ", ", len(treatment2Values)
        print treatment1Values
        print treatment2Values
        print "p-val was ", p_value
        if (p_value < p_threshold):
            plotTo.scatter(genNr, 1, marker='*', c='black')

def calculatePercentiles(values,percentile):
    sortedVals = np.sort(values)
    lower = sortedVals[percentile*sortedVals.size]
    upper = sortedVals[(1-percentile)*sortedVals.size]

    return lower, upper

def gatherAllData(folderPaths,feature):
    """Gathers all data about the given feature under folderPath, and returns it"""


def plotMultirunStatistics(folderPaths,feature, final_generation_number, labels = None, colors=default_colors, markers = default_markers, aspect_ratio = None):
    #Plots the value of some feature over all generations, for several treatments
    if labels:
        label_text = labels
    else:
        label_text=[generateLabel(fp) for fp in folderPaths]
    generations = []
    all_y_values = []
    y_value_names = []

    for folderPath in folderPaths: #Each treatment we are comparing

        currentTreatmentValues = []
        subfolders = [ f for f in listdir(folderPath) if isdir(join(folderPath,f)) ]

        for folder in subfolders:
            if "run" in folder:
                #We have one of our runs.
                populations_files_path = join(join(folderPath,folder),settings.Constants_and_Datastructures.populationsSubFolder)
                try:
                    gens, y_vals, y_val_names = gatherFeatureOverGenerations(populations_files_path,feature, final_generation_number)
                except IOError:
                    print "WARNING: A results folder was not complete. Skipping that result-folder. If this is a final run, please redo it.  Folder is: "
                    print join(folderPath,folder)
                    continue
                generations = gens
                y_value_names = y_val_names
                currentTreatmentValues.append(y_vals[0])
        all_y_values.append(currentTreatmentValues)

    y_value_medians = []
    #Confidence intervals - lower and upper limits
    lower_confidence_intervals = []
    upper_confidence_intervals = []

    #Setting up the plot - a bit complex, since it has several subplots
    gs = gridspec.GridSpec(2, 1, height_ratios=[10, 1])
    mainPlot = plt.subplot(gs[0])
    pValPlot = plt.subplot(gs[1])
    #if(len(all_y_values)==2):
        #TODO: If given more than 2 treatments, I just compare the p-values for the first two.
    plotPValues(all_y_values[0],all_y_values[1],pValPlot)

    #Calculating medians and confidence intervals
    for treatment in all_y_values:
        y_value_medians.append([])
        lower_confidence_intervals.append([])
        upper_confidence_intervals.append([])
        for generation in range(len(generations)):
            try:
                allValuesForThisStatisticAndGeneration = [v[generation] for v in treatment]
            except IndexError:
                print "WARNING: One of the runs did not contain the expected number of generations!"
                continue
            medianForThisStatisticAndGeneration = np.median(allValuesForThisStatisticAndGeneration)
            y_value_medians[-1].append(medianForThisStatisticAndGeneration)
            #print "Gen ", generation, ": ", medianForThisStatisticAndGeneration
            #print "WARNING: Not using bootstrap. Just percentiles for testing."
            #print allValuesForThisStatisticAndGeneration
            #CIs = calculatePercentiles(allValuesForThisStatisticAndGeneration,0.25)
            CIs = bs.ci(allValuesForThisStatisticAndGeneration, statfunction=np.median, n_samples=5000)
            lower_confidence_intervals[-1].append(CIs[0])
            upper_confidence_intervals[-1].append(CIs[1])

    for medians, lower, upper, color, mark, folderName, label in \
            zip(y_value_medians, lower_confidence_intervals, upper_confidence_intervals, colors, markers, folderPaths, label_text):
        #Plotting median
        mainPlot.plot(generations,medians, color= color, marker=mark, label = label, markersize=8)
        #Plotting confidence intervals
        mainPlot.plot(generations,lower, color=color, linestyle="--")
        mainPlot.plot(generations,upper, color=color, linestyle="--")
        mainPlot.fill_between(generations, lower, upper, facecolor=color, alpha=0.1)

    mainPlot.legend(loc=4, fontsize = legend_font_size)
    #plt.title(y_value_names[0])
    mainPlot.set_ylabel(y_value_names[0])
    mainPlot.set_xlabel("Generation")

    if aspect_ratio:
        mainPlot.set_aspect(aspect_ratio)


    #plt.subplots_adjust(hspace=-1.8)
    #plt.tight_layout()
    #plt.show()

def gatherFeatureOverGenerations(folderPath,feature,final_generation=None):
    #Gathers the value of some feature over all generations, for a single run
    files = [ f for f in listdir(folderPath) if isfile(join(folderPath,f)) ]

    y_values = []
    y_values_names = []
    gens = []
    folderComplete = False
    for f in files:
        if not ".pkl" in f:
            continue #Only supporting pickled data for now.
        if "winner_indivs" in f:
            folderComplete=True
        if "gen" in f or "winner_indivs" in f:
            if "gen" in f:
                genNr = int(f.split("gen")[1].split(".")[0])
            elif final_generation!=None and "winner_indivs" in f:
                genNr = final_generation #TODO: Get from file.
            gens.append(genNr)

            fullName = join(folderPath,f)
            data = pickle.load(open(fullName,"rb"))
            population = data[settings.Constants_and_Datastructures.populationName]

            if feature == settings.Constants_and_Datastructures.lengthName:
                if len(y_values) == 0:
                    y_values=[[],[],[]]
                    y_values_names = ["Average Length", "Max Length", "Standard Deviation"]

                populationLengths = []
                for p in population:
                    populationLengths.append(len(p))

                y_values[0].append(float(sum(populationLengths))/float(len(populationLengths)))
                y_values[1].append(max(populationLengths))
                y_values[2].append(np.std(np.array(populationLengths)))

            if feature == settings.Constants_and_Datastructures.hyperVolumeName:
                if len(y_values) == 0:
                    y_values=[[]]
                    y_values_names = ["Hypervolume"]
                hyperVolumeReference = HYPERVOLUME_REF_POINT
                if settings.Constants_and_Datastructures.maxEnergyName in data.keys():
                    hyperVolumeReference[1] = data[settings.Constants_and_Datastructures.maxEnergyName] * 1.01
                    #print "Current hypervolume is ", hyperVolumeReference
                hypVolCalculator = HyperVolume(hyperVolumeReference)
                front = [i.fitness.values for i in population]
                hypVol = hypVolCalculator.compute(front)
                y_values[0].append(hypVol)

            if feature == settings.Constants_and_Datastructures.numMemoizedSolutionsName:
                if len(y_values) == 0:
                    y_values=[[]]
                    y_values_names = ["Elements Memoized"]
                y_values[0].append(data[settings.Constants_and_Datastructures.numMemoizedSolutionsName])



    if not folderComplete: #TODO: Raise here.
        raise IOError("Folder not complete")
        print "This folder was not complete: ", folderPath
    #Code to sort the plotted data
    gensArray = np.array(gens)

    index_to_order_by = gensArray.argsort()
    gensOrdered = gensArray[index_to_order_by]
    y_values_ordered = []

    for valueSet, name in zip(y_values, y_values_names):
        y_values_array = np.array(valueSet)
        ordered = y_values_array[index_to_order_by]
        y_values_ordered.append(ordered)

    return gensOrdered, y_values_ordered, y_values_names


def calculateMedianAndCIs(folder,fileName,variable):
    #Calculates median and CI for the given variable, among all files named fileName under folder.
    files = getFilesRecursively(folder, fileName)
    values = [] #The variable's value in each file.
    for fileName in files:
        loadedData = pickle.load(open(fileName, "rb"))
        values.append(loadedData[variable])

    valueArray = np.array(values)
    median = np.median(valueArray)
    CIs = bs.ci(valueArray, statfunction=np.median, n_samples=5000)

    return median, CIs[0], CIs[1]


def getAllFitnessValues(folder,fileName):
    # Gets fitness values for all individuals recursively located below the given directory,
    # in file named fileName
    files = getFilesRecursively(folder, fileName)
    allSolutions = [] # All solutions from all files.
    for f in files:
        loadedData = pickle.load(open(f, "rb"))
        population = loadedData[settings.Constants_and_Datastructures.populationName]
        for ind in population:
            allSolutions.append(ind.fitness.values)

    return allSolutions

def getAllParetoApproximations(folder,fileName):
    ''' Gets fitness values for all individuals recursively located below the given directory,
     in file named fileName. Returns a list with one entry per pareto approximation.'''
    files = getFilesRecursively(folder, fileName)
    allParetoSets = []
    for f in files:
        loadedData = pickle.load(open(f, "rb"))
        population = loadedData[settings.Constants_and_Datastructures.populationName]
        currentParetoSet = []
        for ind in population:
            currentParetoSet.append(ind.fitness.values)
        allParetoSets.append(currentParetoSet)

    return allParetoSets

def plotAttainmentSurface(folder,fileName):
    allSolutions = getAllFitnessValues(folder, fileName)
    maxEnergy = max(allSolutions, key=lambda item:item[1])[1]
    plan_post_processing.attainmentSurfaceAnalysis.plotSingleAttainment(allSolutions, 0, 1, 0, maxEnergy, title=folder) #TODO: Replace 211 with maxEnergy

def plotAttainmentSurfaceDifference(folder1, folder2, fileName, differenceThreshold):
    treatment1Solutions = getAllFitnessValues(folder1, fileName)
    treatment2Solutions = getAllFitnessValues(folder2, fileName)

    maxEnergy1 = max(treatment1Solutions, key=lambda item:item[1])[1]
    maxEnergy2 = max(treatment2Solutions, key=lambda item:item[1])[1]
    maxEnergyTotal = max(maxEnergy1, maxEnergy2)

    print "max1 was ", maxEnergy1, " and max 2 was ", maxEnergy2, " and total max"

    plan_post_processing.attainmentSurfaceAnalysis.plotAttainmentDifference(treatment1Solutions, treatment2Solutions, 0, 1, 0, maxEnergyTotal,
                                                                            differenceThreshold=differenceThreshold, title = folder1 + " vs " + folder2)


    plan_post_processing.attainmentSurfaceAnalysis.plotAttainmentDifference(treatment2Solutions, treatment1Solutions, 0, 1, 0, maxEnergyTotal,
                                                                            differenceThreshold=differenceThreshold, title = folder2 + " vs " + folder1)


def plotMutlipleParetoFronts(folder,fileName, color="black", marker="o"):
    #Plots pareto fronts from all files named filename under the folder, inserting them all into the same plot.
    files = getFilesRecursively(folder, fileName)
    for f in files:
        loadedData = pickle.load(open(f, "rb"))
        population = loadedData[settings.Constants_and_Datastructures.populationName]
        plotParetoFront2D(population,color=color,marker=marker)
