/*
 * Constants.h
 *
 *This file contains some program-wide constants controlling how coverage estimates are done.
 *
 *  Created on: Jun 2, 2015
 *      Author: kaiolae
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

namespace utility_functions {


	///Constants controlling how the scene is discretized into candidate waypoints.

	//Regulates the spacing between candidate waypoints.
	const double NUM_VIEWPOINTS_INSIDE_INSPECTION_VOLUME = 1000.0;
	const double BOX_SAFETY_MARGIN = 2.0; ///The closest we allow any candidate waypoint to be to the inspection target.
	///How much space we add around the targets' bounding box, in order to allow candidate waypoints at a distance from the target.
	///Higher values increase search complexity, while lower values restrict the potential solutions. A reasonable value is somewhere around the
	///range of the AUV's sensor.
	const double BOX_PADDING = 4.0;

	//The minimum distance any waypoint should be from z=0 (the bottom of the 3D model). This can compensate for unevenness in the ocean floor.
	const double MIN_DIST_FROM_BOTTOM = 3.0;

	///Which cameras should we simulate? We can choose one or more. Covering without any cameras does not make sense
	///In some cases, maybe we only want to scan from the side or only from above - and this makes it that each camera can be independently activated.
	const bool FORWARD_CAMERA_ACTIVE = true;
	const bool DOWNWARD_CAMERA_ACTIVE = true;

	///Controlling the size of images exported from OSG
	const int IMG_WIDTH = 1024;
	const int IMG_HEIGHT = 1024;

	///Scaling factors for the different actions requiring the robot to spend energy.
	///At the moment, moving 20 meters has the same cost as a 180 turn, and 10 meters is the same as a 90 degree.
	const double translationEnergyFactor = 0.1;
	const double rotationEnergyFactor = 1;

	///This constant decides the max energy any plan can have. The calculation is:
	///maxAllowedEnergy = maxEnergyMultiplier*length(longestCirclingPlan)
	//This could allow a broad search at first, and later a more narrow focus. One implementation could be to always send the generation number (in percentage,
	// so gen 0 = 0, genMax = 1)from Python, and use this in an annealing-formula which modifies the values of the constants below.
	const double maxEnergyMultiplier = 1.0;//0.5;//1.5;
	const double COLLISION_PENALTY_MODIFIER = 2.0; //0.5
	const double EDGE_SAFETY_BUFFER = 1.5; ///<The closest we want the center of any edge to be to the inspection target. Used for collision detection.
	const osg::Vec3d UP_VECTOR = osg::Vec3d(0,0,1); ///< A vector showing the up-direction.

	///Default parameters for estimates evaluating as if the sensor was a camera. Can be overridden in the constructor to CameraEstimator.
	const int DEFAULT_CAM_HEIGHT = 1024;
	const double DEFAULT_SAMPLING_INTERVAL_CAM = 2.0;//2.0;

	///These data are from the actual sheets on the robot. These default values can be overridden by the user if he sets up the camera.
	const double CAMERA_NEAR_PLANE_DIST = 0.1;
	const double CAMERA_FAR_PLANE_DIST = 10;
	const double FOV_VERTICAL = 46.0;
	const double FOV_HORIZONTAL = 46.0;


}



#endif /* CONSTANTS_H_ */
