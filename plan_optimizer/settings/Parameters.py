import settings.Constants_and_Datastructures as const

__author__ = 'kaiolae'

# ------------------Evolutionary parameters----------------------------

NUM_INDIVS = 40  # Number of individuals. Has to be divisible by 4.
NUM_GENERATIONS = 400 # Number of generations. 100 - 1000 is reasonable, depending of how much time you have.
STORAGE_INTERVAL = 10  # If given, we store the population every nth generation to a file named "genN.pkl"

#  Probability per individual. Mutates the sequence of viewpoints in the plan. TODO: Consider a swap operator.
WAYPOINT_MUTATION_PROBABILITY = 0.1
MANDATORY_PART_SWAP_PROBABILITY = 0.1  # Only relevant for "interleaved" genotypes.
# Probability of changing angles in the plan. Probability per waypoint. Only relevant when optimizing viewing angles.
ANGLE_MUTATION_PROBABILITY = 0.01

# Parameters controlling the likelihood and impact of real-valued mutations.
REAL_VALUED_MUTATION_MEAN = 0
REAL_VALUED_MUTATION_SIGMA = 0.1
REAL_VALUED_MUTATION_PROBABILITY_PER_GENE = 0.01

CROSSOVER_PROB = 0.1

# -----------------------------Plan initialization parameters---------------------------------

# If seeding is active: Chance that an individual in the initial population will be drawn from the seeds. If not seeded, it will be randomly generated.
USE_PLAN_SEED_CHANCE = 0.35

# Limits of the sizes of random plans we generate in the initial population. Keeping it relatively low, since long plans with many collisions will not be valid anyway.
MIN_PLAN_SIZE = 2 # Since at least two waypoints are needed to generate an edge.
MAX_PLAN_SIZE = 15

# ----------------------------Plan Encoding and constraints-------------------------------------

# Lets us set a mandatory starting place for all plans. Set it to a (x,y,z) vector to enable.
# If set to None, plans do not all start in the same location.
PLAN_ORIGIN = None
PLAN_LOOPS_AROUND = False  # If True, the plan is forced to finish where it started.

# ------------------------------------Parameters you probably do not want to change.----------------------------------

# Reference point for the hypervolume indicater. Used to plot performance
HYPERVOLUME_REF_POINT = [1.01,215]

# Parameters concerning the robotic simulation
# If true, we make the simplifying assumption that the robot always looks "straight ahead".
# If false, viewing directions are also optimized (have not had success optimizing these yet)
SIMPLIFIED_VIEW_ANGLE = True #NOTE: I have not had success in planning with this variable set to False. May be best to keep it as True.

# Parameters of the sensor and AUV

#Camera params are: 1. Distance between snapshots, 2. FOV x, 3. FOV y, 4. Image height (pixels), 5. Distance to near plane, 6. Distance to far plane.
# 7. Front Camera Active? 8. Camera Below Active?
DOWN_CAM_ACTIVE = 1
SENSOR_PARAMETERS = [2.0, 46.0, 46.0, 1024, 0.1, 10, 1, DOWN_CAM_ACTIVE]

# Speeds up evaluation by memoizing results. Probably good idea to keep this active.
USING_EDGE_MEMOISATION = True
