from settings.Parameters import MIN_PLAN_SIZE

__author__ = 'kaiolae'
# The mutation and recombination operators that I use
import copy
import random

import deap.tools.mutation as mut
import settings.runtime_specified_parameters
import settings.Constants_and_Datastructures

# Mutation of waypoints
def mut_delete_waypoint(individual):
    if len(individual) > MIN_PLAN_SIZE: # We do not allow deletions making plans with sizes below this.
        individual.pop(random.randrange(len(individual)))
    else:
        return False #Indicades that delete was unsuccesful.

    return True #Indicades that delete was succesful.

def mutate_waypoints(individual,toolbox):
    # Mutates the plan by inserting, deleting viewpoints in the plan
    if random.random()<0.5:
        # Insert viewpoint
        return mut_insert_waypoint(individual,toolbox)

    else:
        # Remove viewpoint
        return mut_delete_waypoint(individual)


def mut_insert_waypoint(individual,toolbox):
    """Inserts a random viewpoint at a random view in the plan."""
    if settings.runtime_specified_parameters.params.SIMPLIFIED_VIEW_ANGLE:
        new_point = [random.randrange(0, settings.runtime_specified_parameters.num_potential_viewpoints)]
    else:
        # Newly inserted points always look "straight ahead". TODO: Could also consider a small random offset.
        new_point = [random.randrange(0, settings.runtime_specified_parameters.num_potential_viewpoints), 0]
    individual.insert(random.randrange(len(individual)+1),new_point)
    return True

# Mutation of viewing angles of existing waypoints
def mutateViewingAngles(individual):
    if settings.runtime_specified_parameters.params.SIMPLIFIED_VIEW_ANGLE:
        print "WARNING: viewing angles are not being optimized, but their mutator is being called."
        return

    angles = [i[1] for i in individual]
    #Mutating with Deb's polynomial mutation. The value for eta was guesstimated from the paper: http://www.egr.msu.edu/~kdeb/papers/k2012016.pdf
    anglesBefore = copy.deepcopy(angles)
    mut.mutPolynomialBounded(angles, POLY_MUTATION_ETA_PARAMETER, -1, 1,
                             settings.runtime_specified_parameters.params.ANGLE_MUTATION_PROBABILITY)

    for waypointNr in range(len(individual)):
        individual[waypointNr][1] = angles[waypointNr]

    #Return value indicates whether or not anything was mutated.
    if anglesBefore!=angles:
        return True
    else:
        return False


def cleanUpIndividual(individual):
    # Fixes up the individual, in case mutation and crossover has had any erroneous effects.
    prevWaypoint = -1
    waypointsToRemove = []
    counter = 0
    newWaypointsList = []
    #Generating a new list with only the valid plan parts.
    for planPart in individual:
        if settings.runtime_specified_parameters.params.SIMPLIFIED_VIEW_ANGLE:
            currentWaypoint = planPart
        else:
            currentWaypoint = planPart[0]
        if currentWaypoint != prevWaypoint:
            newWaypointsList.append(planPart)

        prevWaypoint = currentWaypoint
        counter+=1

    if len(newWaypointsList) != len(individual):
        print "Had to clean up this genotype: ", individual
        #If we fixed anything, replace the list-part of the individual.
        individual[:] = newWaypointsList
        print "After clean: ", individual

