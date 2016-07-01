/*
 * PlanCoverageEstimator.cpp
 *
 *  Created on: Apr 20, 2015
 *      Author: kaiolae
 */

#include "PlanCoverageEstimator.h"

#include <boost/exception/diagnostic_information.hpp>
#include <osg/LightModel>
#include <osg/ShapeDrawable>
#include <osgDB/ReadFile>
#include <osgText/Text>
#include <osgViewer/Viewer>

#include "../../Utility_Functions/src/CameraEstimator.h"
#include "../../Utility_Functions/src/HelperMethods.h"
#include "../../Utility_Functions/src/KeyboardInputHandler.h"
#include "../../Utility_Functions/src/OsgHelpers.h"
#include "PlanEnergyEvaluator.h"
#include "PlanInterpreterBoxOrder.h"
#include "../../Utility_Functions/src/SceneKeeper.h"
#include "../../Utility_Functions/src/Constants.h"

using namespace utility_functions;
namespace evolutionary_inspection_plan_evaluation {

///Used from the Python interface.
std::vector<std::vector<double> > viewMatrixSelector(
		std::string sceneFileName) {
	return utility_functions::viewMatrixSelector(sceneFileName);
}

void PlanCoverageEstimator::storePlanImage(const std::vector<std::vector<double> >& plan, const std::vector<std::vector<double> >& viewMatrix, std::string& storagePath) {
	osg::ref_ptr<osg::Group> drawable = new osg::Group; //The drawable to plot inside our plan
	evaluatePlan(plan, false, image, true, drawable);
	osg::Matrixd vm = convertNestedVectorToMatrix(viewMatrix);
	storeViewToFile(drawable, vm, storagePath);
}


boost::dynamic_bitset<> PlanCoverageEstimator::evaluateMemoisedPlan(const std::vector<std::vector<double> >& plan){

	osg::Vec3d previousLocation;
	std::string previousPlanPartName = "None";
	if (startLocation != nullptr) {
		previousPlanPartName = "start";
		previousLocation = osg::Vec3d((*startLocation)[0], (*startLocation)[1],
				(*startLocation)[2]);
	}

	boost::dynamic_bitset<> observedColors(sceneKeeper->getTriangleCount());
	int memoizedCount = 0;
	int notmemoizedCount = 0;
	for (size_t i = 0; i < plan.size(); i++) {
		//Each part of a plan is a vector of doubles - representing a single viewpoint. The first element of the vector is the position of that viewpoint.
		std::vector<double> currentPlanPart = plan[i];
		if (i != 0) {
			previousPlanPartName = vectorToString(plan[i - 1], 1);
		}
		std::string currentPlanPartName = vectorToString(currentPlanPart,
				currentPlanPart.size());
		std::string memoizationIndex = previousPlanPartName + "-"
				+ currentPlanPartName; //The string we use to store and look up the current edge.

		//std::cout << "Evaluating "<< memoizationIndex << std::endl;

		boost::dynamic_bitset<> currentlyObservedColors(sceneKeeper->getTriangleCount());
		if (memoisedEdgesBitset->find(memoizationIndex)	== memoisedEdgesBitset->end()) {
			notmemoizedCount+=1;
			//Element not memoized. Calculating manually.
			osg::ref_ptr<osg::Vec3dArray> decodedPositions = new osg::Vec3dArray;
			osg::ref_ptr<osg::Vec3dArray> decodedAngles = new osg::Vec3dArray;
			if (previousPlanPartName != "start"	&& previousPlanPartName != "None") {
				previousLocation = planInterpreter->getPositionFromGene(plan, i - 1);
			}
			if (previousPlanPartName == "None") {
				//In the special case that this is the first point, we have no previous point.
				//This will normally result in no decoded edges (which will simply result in no observed primitives),
				//but in the case that the plan starts with a loop, we will have edges also in this first part of the plan.
				planInterpreter->InterpretOnePlanPoint(nullptr, currentPlanPart, *decodedPositions, *decodedAngles);

			} else {
				decodedPositions->push_back(previousLocation); //Move starts at previous location
				planInterpreter->InterpretOnePlanPoint(&previousLocation,currentPlanPart, *decodedPositions, *decodedAngles);
			}
			if (decodedPositions->size() < 2) {
				//The decoding only gave us a single waypoint. No edge to traverse. This should only happen when decoding the first point
				assert(i == 0);
				continue;
			}
			//Fetched the next viewpoints and angles (often this will just be one viewpoint). Now, recording and storing their coverage.
			for (size_t edgeNr = 0; edgeNr < decodedAngles->size(); edgeNr++) {
				//Iterates over all the edges we just decoded (usually just one, unless we decoded a complete loop).
				osg::Vec3d previousNode = decodedPositions->at(edgeNr);
				osg::Vec3d nextNode = decodedPositions->at(edgeNr + 1);
				osg::Vec3d angle = decodedAngles->at(edgeNr);
				cam_estimator->GetColorsDuringTraversal(previousNode, nextNode,
						angle, coloredScene, currentlyObservedColors);
				observedColors |= currentlyObservedColors; //bitwise OR
				(*memoisedEdgesBitset)[previousPlanPartName + "-"
						+ currentPlanPartName] = currentlyObservedColors;
			}

		} else {
			memoizedCount+=1;
			//Element memoized. Fetching values.
			boost::dynamic_bitset<> memoizedResult =
					memoisedEdgesBitset->at(memoizationIndex);
			observedColors |= memoizedResult;
		}

	}

	return observedColors;

}

std::vector<double> PlanCoverageEstimator::evaluateEmptyPlan() const{
	//Immediately giving the default score to empty plans can speed evaluation up.
	double objectiveScores[] = { 1.0, 0 }; //The score for an empty plan. No coverage, no energy spent.
	std::vector<double> scoreVector(objectiveScores,
			objectiveScores + sizeof(objectiveScores) / sizeof(double));
	return scoreVector;
}

boost::dynamic_bitset<> PlanCoverageEstimator::evaluatePlanInternal(const std::vector<std::vector<double> >& plan,osg::ref_ptr<osg::Geode> geode) const{

	osg::ref_ptr<osg::Vec3dArray> plannedPositions = new osg::Vec3dArray(); //All points we will visit
	osg::ref_ptr<osg::Vec3dArray> plannedAngles = new osg::Vec3dArray(); //The angle the AUV will point towards in those positions.
	osg::ref_ptr<osg::Group> textureRoot(new osg::Group); //For plotting only the texture showing what the simulated camera sees.
    planInterpreter->InterpretPlan(plan, *plannedPositions, *plannedAngles);
	osg::Vec3d currentLocation = (*plannedPositions)[0];

	boost::dynamic_bitset<> observedColors(sceneKeeper->getTriangleCount());
	//Going through each edge, calculating it's coverage.
	for (unsigned int i = 0; i < plannedAngles->size(); i++) {
		osg::Vec3d nextLocation = (*plannedPositions)[i + 1]; //We have one more position than angles, as positions refer to nodes, and angles to edges.
		osg::Vec3d sensorHeading = (*plannedAngles)[i];
		cam_estimator->GetColorsDuringTraversal(currentLocation, nextLocation,
				sensorHeading, coloredScene, observedColors, textureRoot);

		currentLocation = nextLocation;
	}

	if (geode != nullptr) {
		drawTrajectory(plannedPositions, plannedAngles, *geode); //Drawing the AUV's path to the geode.
	}
	return observedColors;
}

PlanCoverageEstimator::~PlanCoverageEstimator() {
	//delete planInterpreter;
	delete sceneKeeper;
	delete energyEvaluator;
	delete cam_estimator;
}

//Only valid if we have a "Box-Order" interpretation of our plan.
int PlanCoverageEstimator::getNumberOfBoxes() const{

	assert(planInterpreter != nullptr);
	return planInterpreter->getNumberOfBoxes();
}

PlanCoverageEstimator::PlanCoverageEstimator(const std::string& sceneFileName,
		const std::vector<double>& sensorSpecs, bool postProcessing/*=false*/, const std::vector<double>* startLocation/*=nullptr*/, bool planLoopsAround /*= false*/,
		bool printerFriendly/*=true*/)
	:startLocation(startLocation),
	 planLoopsAround(planLoopsAround){
	std::cout << "Loading scene" << std::endl;
	this->printerFriendly = printerFriendly;
	sceneKeeper = new SceneKeeper(sceneFileName);
	sceneKeeper->countTriangles();

	memoisedEdgesBitset = new std::map<std::string, boost::dynamic_bitset<> >;
	coloredScene = sceneKeeper->colorEachTriangleDifferently();

	cam_estimator = new CameraEstimator(sensorSpecs);

	bool usingMaxEnergy = true;
		this->boxes = new osg::Vec3dArray();
		planInterpreter = new PlanInterpreterBoxOrder(*sceneKeeper, boxes, *cam_estimator, coloredScene, startLocation, postProcessing);
		if (!postProcessing) {
			usingMaxEnergy = true;
			planInterpreter->ProduceSimpleCirclingPlans();
		} else {
			//In post-processing runs, we do not estimate the max energy, to save time.
			usingMaxEnergy = false;
			planInterpreter->setPostProcessing(true);
		}
	energyEvaluator = new PlanEnergyEvaluator(usingMaxEnergy, this,	planInterpreter, *sceneKeeper);

}



	//This way of passing the how_to_plot arguement is not very elegant (I would prefer an enum).
	//However, this method needs to be callable from Python - therefore, passing as a string works better.
std::vector<double> PlanCoverageEstimator::evaluatePlan(
			const std::vector<std::vector<double> > &plan, bool memoization,
			plotting_style how_to_plot, bool disableEnergyLimit/* = false*/,
			osg::ref_ptr<osg::Group> returnedDrawable/*=nullptr*/) {

	if(how_to_plot==image && returnedDrawable== nullptr){
		throw std::logic_error("ERROR! Asked to plot image, but not given an OSG-drawable to plot to.");
	}

	//std::cout << "Evaluating plan: " << plan << std::endl;
	if (plan.size() == 0) {
		return evaluateEmptyPlan();
	}

	//Making a copy of the plan that we can modify - so we are sure never to mess with the genotype.
	std::vector<std::vector<double> > planCopy = plan;
	//If we want to enforce looping, we simply reinsert the first waypoint in the end.
	if(planLoopsAround){
		planCopy.push_back(plan[0]);
	}

	double energyUsed = energyEvaluator->decodeAndCalculateEnergyUsage(planCopy);
	if(!disableEnergyLimit && !energyEvaluator->energyWithinBounds(energyUsed)) {
			//std::cout << "Current plan: " << plan << std::endl;
			double objectiveScores[] = {1, energyUsed}; //For both objectives: 0 is best score.
			std::vector<double> scoreVector(objectiveScores,
											objectiveScores + sizeof(objectiveScores) / sizeof(double));
			return scoreVector;
	}

	osg::ref_ptr<osg::Group> root = nullptr; //For plotting
	osg::ref_ptr<osg::Geode> geode = nullptr; //For plotting
	osg::ref_ptr<osg::Group> textureRoot = nullptr; //For plotting only the texture showing what the simulated camera sees.
	if (how_to_plot!=nothing) {
		root = new osg::Group;
		geode = new osg::Geode();
		textureRoot = new osg::Group;
	}

	//We use one of these to measure coverage.
	boost::dynamic_bitset<> observedColors; ///<Will contains zeros for all colors that have not been observed by the camera, and one for those that have.
	std::set<int> coveredPrimitives;

	if (memoization) {
		observedColors = evaluateMemoisedPlan(planCopy);
	} else {
		observedColors = evaluatePlanInternal(planCopy, geode);
	}

	double coverageScore;
	if (how_to_plot!=nothing) {
		coverageScore = sceneKeeper->calculateCoverage(observedColors, geode,
				printerFriendly);

	} else {
		coverageScore = sceneKeeper->calculateCoverage(observedColors);
	}
	double objectiveScores[] = { coverageScore, energyUsed }; //For both objectives: 0 is best score.

	if (how_to_plot==normal) {

		osgViewer::Viewer viewer;
		//viewer.getCamera()->setClearColor(osg::Vec4(1.0f,1.0f,1.0f,1.0f)); //Making background white.
		osg::ref_ptr<osg::Group> fullScene = new osg::Group;

		fullScene->addChild(geode);
		viewer.setSceneData(fullScene.get());
		viewer.run();
	} else if(how_to_plot==image) {
		returnedDrawable->addChild(geode);
	}
	else if(how_to_plot==circulating){
		run_rotating_camera(geode);
	}

	std::vector<double> scoreVector(objectiveScores,
			objectiveScores + sizeof(objectiveScores) / sizeof(double));

	//std::cout << "Plan score was: " << scoreVector << std::endl;
	return scoreVector;

}

int PlanCoverageEstimator::updateMemoisedEdges(
		const std::vector<std::vector<std::vector<double> > >& allSolutions) {
	//std::cout << "population is: " << std::endl;
	//Finding all edges in the current population.
	std::set<std::string> allEdgesInPopulation;
	for (std::vector<std::vector<std::vector<double> > >::const_iterator popItt =
			allSolutions.begin(); popItt != allSolutions.end(); popItt++) {
		//std::cout << "Next individual: ";
		std::vector<std::vector<double> > currentSolution = *popItt;
		std::string previousPlanPartName = "start";
		for (std::vector<std::vector<double> >::iterator planItt =
				currentSolution.begin(); planItt < currentSolution.end();
				planItt++) {
			std::vector<double> nextStep = *planItt;
			std::string currentPlanPartName = vectorToString(nextStep,
					nextStep.size());
			std::string memoizationIndex = previousPlanPartName + "-"
					+ currentPlanPartName; //The string we use to store and look up the current edge.
			allEdgesInPopulation.insert(memoizationIndex);

			//std::cout << memoizationIndex << " - ";
			std::vector<double> prevStep = nextStep;
			previousPlanPartName = vectorToString(prevStep, 1);
		}
	}
	//std::cout<<std::endl;
	std::set<std::string> edgesToRemove;
	//Removing any memoized edges not in the population.
	for (std::map<std::string, boost::dynamic_bitset<> >::iterator memEdges2 =
			memoisedEdgesBitset->begin();
			memEdges2 != memoisedEdgesBitset->end(); memEdges2++) {
		std::string edgeName = memEdges2->first;
		if (allEdgesInPopulation.find(edgeName) == allEdgesInPopulation.end()) { //Find in set has logarithmic complexity.
			edgesToRemove.insert(edgeName);
		}
	}

	for (std::set<std::string>::iterator deletedEdge = edgesToRemove.begin();
			deletedEdge != edgesToRemove.end(); deletedEdge++) {
		memoisedEdgesBitset->erase(*deletedEdge);
	}

	return memoisedEdgesBitset->size();
}

std::vector<std::vector<std::vector<double> > > PlanCoverageEstimator::getSimplePlans() const {
	return planInterpreter->generateOrGetCompleteCirclingPlans();
}

void PlanCoverageEstimator::interpretAndExportPlan(
		const std::vector<std::vector<double> >& plan,
		std::vector<std::vector<double> >& plannedPositions,
		std::vector<std::vector<double> >& plannedAngles) const{
	osg::ref_ptr<osg::Vec3dArray> osgPlanPos = new osg::Vec3dArray();
	osg::ref_ptr<osg::Vec3dArray> osgPlanOrientations = new osg::Vec3dArray();
	planInterpreter->InterpretPlan(plan, *osgPlanPos, *osgPlanOrientations);

	plannedPositions = convertOsgVec3dArrayToVector(*osgPlanPos);
	plannedAngles = convertOsgVec3dArrayToVector(*osgPlanOrientations);
}


	double PlanCoverageEstimator::getMaxAllowedEnergy() const {
		return energyEvaluator->getMaxAllowedEnergy();
	}

} /* namespace evolutionary_inspection_plan_evaluation */
