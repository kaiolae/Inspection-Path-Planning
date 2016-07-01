/*
 * PlanCoverageEstimator.h
 *
 * Functionality for estimating how much of a scene a given plan (sequence of waypoints and angles) will cover.
 *
 *  Created on: Apr 20, 2015
 *      Author: kaiolae
 */

#ifndef PLANCOVERAGEESTIMATOR_H_
#define PLANCOVERAGEESTIMATOR_H_

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <map>
#include <osg/Array>
#include <osg/Geode>
#include <osg/Group>
#include <osg/ref_ptr>
#include <set>
#include <string>
#include <vector>

namespace utility_functions{
	class SceneKeeper;
	class CameraEstimator;
}


enum plotting_style {
	normal, //Show 3D figure (and plan) on screen, and allow the user to move them around with OSG's controllers.
	circulating, //Auto-circulates around the plan, good way to create nice videos/demos of plan.
	image, //Stores the plan as image, instead of showing on screen.
	nothing //Plots nothing, stores nothing
};


namespace evolutionary_inspection_plan_evaluation {

//Forward declarations
class PlanEnergyEvaluator;
class PlanInterpreterBoxOrder;


	std::vector<std::vector<double> > viewMatrixSelector(std::string sceneFileName);

/**
 * This class is the main interface to the evaluation of inspection plans. One Estimator should be constructed for the entire optimization run, and every time a plan
 * is evaluated, the Estimator-object's evaluatePlan or evaluatePlanWithMemoization should be called.
 * Calling updateMemoisedEdges is also useful if one is doing memoisation, to avoid running out of memory.
 */
class PlanCoverageEstimator {


private:
	const std::vector<double>* startLocation; 		///<The starting point in the scene of this plan.
	osg::ref_ptr<osg::Geode> coloredScene;			///<The inspection target, with each triangle colored differently. Used to estimate what triangles the camera covers.
	utility_functions::SceneKeeper *sceneKeeper;						///<Helper class for methods related to operations on the scene graph.
	osg::ref_ptr<osg::Vec3dArray> boxes;			///<The "boxes" that we consider visiting around the inspection target. In other words, the candidate waypoints for plans.
	PlanInterpreterBoxOrder* planInterpreter; 				///<The class that decodes our plan from the optimizers' representation into a sequence of positions and angles.
	PlanEnergyEvaluator* energyEvaluator;			///The class that helps us calculate the energy spending of plans.
	bool printerFriendly;							///If true, we optimize colors of result images for B/W printing.
	bool planLoopsAround;

	///Parameters specific for camera-based coverage. This type accumulates a list of "colors", each one representing one covered triangle in the model.
	utility_functions::CameraEstimator* cam_estimator;	///<An object estimating a camera, used to estimate what the camera sees while traversing an edge in the plan.
	///This holds all the covered colors in all the edges in the current population, indexed by edge name. Helps us avoid many costly recalculations.
	std::map<std::string,boost::dynamic_bitset<> >* memoisedEdgesBitset;

	/**
	 * Evaluates the plan rapidly, by using memoized subparts. Also memoizes new subparts as it goes.
	 * Note that the public interface towards this is through the evaluatePlanWithMemoization method.
	 * Note that only coverage (not energy) is memoised, as energy usage is rapidly calculated and we want to
	 * calculate energy before checking coverage since that may help us discard unfeasible, long plans without beginning to evaluate them.
	 * @param plan The plan to be evaluated
	 * @return The set of all colors the AUV saw carrying out this plan.
	 */
	boost::dynamic_bitset<> evaluateMemoisedPlan(const std::vector<std::vector<double> >& plan);

	/**
	 * Evaluates the plan, storing a set of all the colors or primitives that were seen.
	 * Note that the public interface towards this is through the evaluatePlan method.
	 * @param plan The plan to be evaluated
	 * @param geode A drawable that we store information about what the AUV saw into.
	 * @return The set of all colors the AUV saw carrying out this plan. Only used when we "evaluate as camera".
	 */
	boost::dynamic_bitset<> evaluatePlanInternal(const std::vector<std::vector<double> >& plan, osg::ref_ptr<osg::Geode> geode) const;

	/**
	 * Gives the default score to an empty plan
	 * @return Default score for empty plan
	 */
	std::vector<double> evaluateEmptyPlan() const;

public:
	/**
	 * Constructor for the plan coverage estimator. The external planner calls this constructor, which sets everything up and prepares everything for making each plan estimation
	 * rapid. Should only be called once for the entire optimization run.
	 * @param sceneFileName The path of the 3D-model file containing scene we are generating a plan for. The file has to be in an OSG-compatible format.
	 * @param startLocation The (x,y,z) location in the scene where the robot starts. Should be the spot where we assume the robot will "arrive".
	 * @param sensorSpecs Specifications for our sensor. See CameraEstimator.h for details
	 * @param printerFriendly If true, the color scheme is optimized for Black-and-White printing. If false, it is optimized for color view.
	 */
	PlanCoverageEstimator(const std::string& sceneFileName, const std::vector<double>& sensorSpecs, bool postProcessing = false,
			const std::vector<double>* startLocation = nullptr, bool planLoopsAround = false, bool printerFriendly = true);


	/**
	 * This method should be called regularly when evaluating plans with memoization, to avoid the memory of solution subparts growing towards infinity.
	 * It takes the current population of solutions, and removes all plan parts that are not inside that population from the set of remembered parts.
	 * @param allSolutions The set of all solutions we want to keep remembering plan parts for. For an evolutionary run, this could be the entire current generation.
	 */
	int updateMemoisedEdges(const std::vector<std::vector<std::vector<double> > >& allSolutions);

	/**
	 * Returns the number of "boxes" our planner needs to consider, that is, the number of different locations we have discretized our planning space into.
	 * This will ensure the planner can stick to only valid plans: As we have N different locations, the planner should only consider plans locations with ID between 0 and N-1.
	 * @return The number of "boxes", or different locations, we are allowing our planner to consider.
	 */
	int getNumberOfBoxes() const;

	~PlanCoverageEstimator();

	/**
	 * The "simple plans" are plans generated by a static procedure (not an optimizer), that often show a reasonable performance. Can be useful as seeds for further optimization.
	 * @bool increase_num_z_levels Doubles the number of potential z-levels in these simple plans. This is only used for testing of simple circling plans:
	 * By having more z-levels to circle around, we can capture a greater diversity in such plans. For optimization and seeding, however, this should not be used.
	 */
	std::vector<std::vector<std::vector<double> > > getSimplePlans() const;

	/**
	 * Stores a picture of the plan, which is helpful for analysis of the optimized results.
	 * @param plan An inspection plan
	 * @param viewMatrix The view we would like to take the picture of the plan from
	 * @param storagePath The file name of the file we store the picture into
	 */
	void storePlanImage(const std::vector<std::vector<double> >& plan, const std::vector<std::vector<double> >& viewMatrix, std::string& storagePath);

	double getMaxAllowedEnergy() const;
	/**
	 * Evaluates the given plan, and returns its evaluation along all relevant objectives.
	 * @param plan a vector of plan elements. The representation can vary, and this is handled by having a separate InterpretPlan class that translates
	 * the plan representation into a sequence of locations and angles.
	 * @param memoization true if we want to memoized parts of the plan for later, speeding up execution. Note that the
	 * plan encoding needs to support this for it to work.
	 * @param shouldPlot If true, the osgViewer is called to show the plan to the user. Otherwise, the plan is only evaluated, and not displayed.
	 * @param returnedDrawable If given, we write the plotted scene to this object instead of showing it on the screen.
	 * This thus enables storing scenes instead of displaying. Note: This parameter is not available through the Python interface.
	 * @bool disableEnergyLimit If True, we do not perform a check that the plan stays within an energy limit.
	 * @return A tuple of (coverage, energy_usage). Returning a vector (rather than a pair), so we can be able to support more than two objectives in the future.
	 */
	std::vector<double> evaluatePlan(const std::vector<std::vector<double> > &plan, bool memoization,
									 plotting_style how_to_plot, bool disableEnergyLimit = false,
									 osg::ref_ptr<osg::Group> returnedDrawable = nullptr);

	/**
	 * Interprets the encoded plan, and returns the resulting waypoint positions and AUV orientations.
	 * This is useful when we want to export a plan to another program or store calculated waypoints and orientations to file.
	 * @param plan An encoded plan
	 * @param plannedPositions The decoded positions of waypoints in the plan
	 * @param plannedAngles The decoded AUV orientations on edges in the plan
	 */
	void interpretAndExportPlan(const std::vector<std::vector<double> >& plan, std::vector<std::vector<double> >& plannedPositions, std::vector<std::vector<double> >& plannedAngles) const;


};

} /* namespace evolutionary_inspection_plan_evaluation */

#endif /* PLANCOVERAGEESTIMATOR_H_ */
