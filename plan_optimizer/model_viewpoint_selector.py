#!/usr/bin/env python
#To store pictures of plans, one has to determine a viewpoint that the picture will be taken from.
#This script allows you to store a viewpoint for a 3D structure, which will be applied in the future
#any time a picture of that structure is stored.


import argparse
import store_plan_images


def parse_input_arguments():
    # Parsing user inputs:
    parser = argparse.ArgumentParser(
        description='Allows the user to select a viewpoint for a given 3D structure.\n This viewpoint will then be used whenever this structure (or a plan surrounding it) is stored as an image.')

    # Mandatory Arguments for all plan types
    mandatory_arguments = parser.add_argument_group("Mandatory Arguments")
    mandatory_arguments.add_argument('-i','--input_file', type=str, required=True,
                                     help='Path to a 3D structure that can be opened in openscengraph. Many mesh-based 3D formats are supported.')

    return parser.parse_args()



if __name__ == "__main__":
    args = parse_input_arguments()
    store_plan_images.selectViewpoint(args.input_file)