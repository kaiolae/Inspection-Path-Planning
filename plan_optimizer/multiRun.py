#!/usr/bin/env python
import os

__author__ = 'kaiolae'
#For running a single experiment multiple times - when we want to gather statistics.

import subprocess

from settings import Utilities
from settings import Parameters as parameterFile
import settings.Constants_and_Datastructures as constants

numRuns = 20
paramsFile = "Parameters.py"

if __name__ == "__main__":

    args = Utilities.parse_inspection_planning_arguments()
    print "Outfolder: ", args.output_folder
    Utilities.store_run_info(args.output_folder, args.mandatory_plan_parts_folder)

    for i in range(numRuns):
        outputFolder = args.output_folder+"/run" + str(i) + "/"
        optimizer_call = ['python', os.path.join(constants.SOURCE_CODE_ROOT,'optimize_coverage.py'), '-i', args.input_model_path, '-o', outputFolder,  "--multirun"]
        if args.disable_seeding:
            optimizer_call.append("--disable_seeding")
        subprocess.call(optimizer_call)
