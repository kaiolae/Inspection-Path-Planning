/*
 * PlanEnergyEvaluator.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: kaiolae
 */

#include "PlanEnergyEvaluator.h"

#include <iterator>

#include "../../Utility_Functions/src/Constants.h"
#include "../../Utility_Functions/src/OsgHelpers.h"
#include "PlanCoverageEstimator.h"
#include "../../Utility_Functions/src/SceneKeeper.h"
#include "PlanInterpreterBoxOrder.h"

using namespace utility_functions;
namespace evolutionary_inspection_plan_evaluation {

double PlanEnergyEvaluator::decodeAndCalculateEnergyUsage(const std::vector<std::vector<double> >& plan){
	osg::ref_ptr<osg::Vec3dArray> plannedPositions = new osg::Vec3dArray(); //All points we will visit
	osg::ref_ptr<osg::Vec3dArray> plannedAngles = new osg::Vec3dArray();  //The angle the AUV will point towards in those positions.
	planInterpreter->InterpretPlan(plan,*plannedPositions,*plannedAngles);
	return calculateEnergyUsageOfPlan(*plannedPositions, sceneKeeper.getScene(), sceneKeeper.getCollisionPenalty());
}

bool PlanEnergyEvaluator::energyWithinBounds(double energyUsed){
	//Checks if the given plan uses too much energy.
	if(maxAllowedEnergy==-1){ //-1 tells us this check is disabled.
		return true;
	}
	else{
		return energyUsed<=maxAllowedEnergy;
	}
}



void PlanEnergyEvaluator::calculateMaxAllowedEnergyUsage(){
	std::vector<std::vector<std::vector<double> > > simplePlans = coverageEstimator->getSimplePlans();
	//The longest simple plan is always the first one. Note: Others may have higher energy usage due to collisions. But, the plan we really care about is the one that does
	//the most circles around the inspection target - and that is the one at the front.
	double maxEnergyOfSimplePlans = decodeAndCalculateEnergyUsage(simplePlans.front());
	maxAllowedEnergy = maxEnergyOfSimplePlans*maxEnergyMultiplier;
}


} /* namespace evolutionary_inspection_plan_evaluation */
