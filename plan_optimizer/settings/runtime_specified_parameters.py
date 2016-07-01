# Some parameters that are set at runtime, typically just after analyzing the inspection target.
# They are here to be available to multiple different modules.
__author__ = 'kaiolae'

num_potential_viewpoints = None # The number of potential viewpoints to consider in our planning.
max_energy_usage = None # The max energy usage of any allowed plan.
num_memoized_edges = None # The number of edges memoized at any point. Important to keep low enough to avoid memory filling up.
params = None #The module that holds this run's parameters, imported runtime.
algorithm_start_time = -1 #The clock time when the algorithm started.
use_seeds = True