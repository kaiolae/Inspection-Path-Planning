/*
 * PlanInterpreterBoxOrder.h
 *
 *  Created on: May 8, 2015
 *      Author: kaiolae
 */

#ifndef PLANINTERPRETERBOXORDER_H_
#define PLANINTERPRETERBOXORDER_H_

#include <stddef.h>
#include <osg/Array>
#include <osg/BoundingBox>
#include <osg/Node>
#include <osg/ref_ptr>
#include <osg/Vec3d>
#include <vector>
#include <bits/unique_ptr.h>
#include "../../Utility_Functions/src/CameraEstimator.h"

//Forward declarations
namespace utility_functions{
	class SceneKeeper;
}

namespace evolutionary_inspection_plan_evaluation {

/**
 * A plan interpreter which interprets a plan as a sequence of "Boxes" (approximate coordinates) to visit.
 */
class PlanInterpreterBoxOrder{
private:
	const std::vector<double>* startLocation; 	///<Where the plan starts from.
	utility_functions::SceneKeeper& sceneKeeper; ///<The inspection target
	osg::ref_ptr<osg::Vec3dArray> boxCenters; ///<The center locations of the boxes that represent all possible viewpoints

	///A representation of all the boxes, with values:
	///-2 for boxes that are too far away from the structure to consider in our plan
	///-1 for boxes we don't consider for plans because they come too close to (potentially intersecting) the target
	/// the ID of the corresponding box otherwise.
	///outmost vector: each z-level. Next one: Each zy-level. Innermost vector: Each x-position.
	std::vector<std::vector<std::vector<int> > > boxVolume;

	///Plans covering the target by circling around it as proposed by Galceran et al.
	///Vector contains one plan for each z-level we are considering. Combining multiple such "level-plans"
	///can thus give plans covering the entire object.
	std::vector<std::vector<std::vector<double> > > singleLevelCirclingPlans;


	///Multiple plans circling the structure at different levels of depth
	std::vector<std::vector<std::vector<double> > > completeCirclingPlans;

	///Required to decide the best viewing direction as we decode.
	utility_functions::CameraEstimator& camEstimator;
	osg::ref_ptr<osg::Geode> coloredScene;

	///When true, we do a more thorough post-evolution decoding.
	bool post_processing;

	/**
	 * Tells us if a given box in boxVolume is a neighbor of the inspection target - that is, that there is no
	 * other box between it and the inspection target. This is useful to know when we want to generate a plan
	 * circling the target at close distance.
	 * @param x, y, z The location of the queried box in our boxVolume structure
	 * @return true if box neighbors object, false otherwise
	 */
	bool boxNeighborsObject(int x, int y, int z) const;

	/**
	 * Divides the scene into boxes, following the parameters set in the constructor.
	 * @param z_interval_scaling If set less than 1, loops are spaced closer together along the z-axis.
	 * Only used to generate a wider diversity of circling plans for comparisons. During optimization runs, it
	 * should not be used (including for seeding).
	 * @param[out] boxes The list of generated box centers, returned by reference.
	 */
	void divideIntoBoxes(osg::ref_ptr<osg::Vec3dArray> boxes);

	//Preventing copying of this object.
	//Here, I'm disallowing copy-constructors for this object. Since this is a large object, we want to avoid copies, and rather use pointers.
	PlanInterpreterBoxOrder(const PlanInterpreterBoxOrder&) = delete;
	PlanInterpreterBoxOrder& operator=(const PlanInterpreterBoxOrder&) = delete;


	osg::Vec3d calculateSensorDirectionAndNormalize(const osg::Vec3d& prevPosition, const osg::Vec3d& nextPosition, float offsetRadians=0) const;

public:

	/**
	 * Constructor, setting the parameters of the interpreter.
	 * @param startLocation The starting location of plans in the scene. The place where the AUV is expected to "enter" the scene.
	 * If nullptr, we assume we start at the first position given in the plan.
	 * @param scene The inspection target
	 * @param post_processing If true, we are done optimizing plans, and can use more time to interpret the plan more intelligently.
	 */
	PlanInterpreterBoxOrder(utility_functions::SceneKeeper& sceneKeeper, osg::ref_ptr<osg::Vec3dArray> boxes, utility_functions::CameraEstimator & camEstimator,
			osg::ref_ptr<osg::Geode> coloredScene, const std::vector<double>* startLocation = nullptr, bool post_processing = false);


	virtual ~PlanInterpreterBoxOrder();



	/**
	 * Returns the number of candidate waypoints. Necessarry for the plan optimizer, to know the range of box IDs to include in plans.
	 * @return The number of boxes (candidate viewpoints) in this scene.
	 */
	int getNumberOfBoxes() const;

	/**
	 * Takes a plan, represented as a sequence of box IDs, and converts it to a sequence of waypoint locations (representing the center of the boxes) and a sequence of angles.
	 * If we get 2 doubles for each step of the plan: First double in plan is location we move to - second is the angle we point the sensor in.
	 * However, if we get just one double: We assume we always look to the center of the shape we're inspecting, for simplicity.
	 * @param plan The plan, represented as a sequence of box IDs
	 * @param[out] plannedPositions The waypoints this plan will visit, returned by reference
	 * @param[out] plannedAngles The angles the camera will point in along all edges in the plan.
	 */
	void InterpretPlan(const std::vector<std::vector<double> >& plan, osg::Vec3dArray& plannedPositions, osg::Vec3dArray& plannedAngles) const;


	/**
	 * Interprets a single point in the plan, turning the index of the planned point into a position and movement angle
	 * The resulting movement position and angle are appended to the arrays positions and sensorDirections.
	 * @param prevPosition The previous position in the plan. Needed to calculate the AUV orientation while moving towards the next point.
	 * If not given, we assume this is the first point in the plan, and no orientation is calculated.
	 * @param encodedPlanPoint The ID of the point we are interpreting
	 * @param[out] position A vector of plan positions, with the recently decoded one(s) added to the end.
	 * @param[out] sensorDirection A vector of sensor directions with the recently decoded one(s) added to the end.
	 */
	void InterpretOnePlanPoint(osg::Vec3d* prevPosition, const std::vector<double>& encodedPlanPoint, osg::Vec3dArray& positions,
			osg::Vec3dArray& sensorDirections) const;

	osg::Vec3d getPositionFromGene(const std::vector<std::vector<double> >& genotype, size_t geneID) const;

	/**
	 * Returns the center location of a given box
	 * @param id The box ID
	 * @return The center coordinates of the input box
	 */
	osg::Vec3d getBoxPosition(size_t id) const;

	/**
	 * Produces a set of simple plans circling around the current structure at various levels.
	 * The plans are stored into member variable singleLevelCirclingPlans.
	 * Each plan in the stored vector circles the structure at a single depth-level.
	 * This type of plan is inspired by the paper "Coverage Path Planning with Real-time Replanning
	 *  and Surface Reconstruction for Inspection of Three-dimensional Underwater
	 *   Structures using Autonomous Underwater Vehicles", Galceran et al. 2014.
	 */
	void  ProduceSimpleCirclingPlans();


	/**
	 * Uses already generated circling plans for each level (made by function ProduceSimpleCirclingPlans and combines
	 * them into complete inspection plans of different lengths: One plan circling at one level, the next at two levels, etc.
	 * @return A vector with several simple plans circling the structure at different numbers of depth levels.
	 */
	std::vector<std::vector<std::vector<double> > > generateOrGetCompleteCirclingPlans();

	void setPostProcessing(bool postProcessing) {
		post_processing = postProcessing;
	}

	void drawAllBoxes(osg::ref_ptr<osg::Geode> draw_to_geode) const;

};

} /* namespace evolutionary_inspection_plan_evaluation */

#endif /* PLANINTERPRETERBOXORDER_H_ */
