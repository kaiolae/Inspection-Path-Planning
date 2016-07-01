__author__ = 'kaiolae'
from settings.Parameters import SIMPLIFIED_VIEW_ANGLE
from settings.Constants_and_Datastructures import Fitness
import random

#  These are the plan representations: Just a list of waypoint IDs that the plan will visit.
class Individual(list):
    #Individuals are just lists with a few extra features, importantly with a fitness value.

    def __init__(self,individualContents):
        list.__init__(self,individualContents)
        self.fitness = Fitness()
        self.length = len(individualContents)

    @classmethod
    def randInit(cls,length,numBoxes):
        planSteps = []
        for gene in range(length):
            if SIMPLIFIED_VIEW_ANGLE:
                newGene = [random.randrange(0,numBoxes)]
            else:
                newGene = [random.randrange(0,numBoxes), 0] #TODO: Consider small intial random offset.
            planSteps.append(newGene)

        return cls(planSteps)

    @classmethod
    def initializeFromList(cls,seedList):
        if not SIMPLIFIED_VIEW_ANGLE:
            for viewpoint in seedList:
                viewpoint.append(0.0)
        return cls(seedList)