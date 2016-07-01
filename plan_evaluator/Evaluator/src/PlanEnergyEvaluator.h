/*
 * PlanEnergyEvaluator.h
 *
 *  Created on: Jul 31, 2015
 *      Author: kaiolae
 */

#ifndef PLANENERGYEVALUATOR_H_
#define PLANENERGYEVALUATOR_H_

#include <stddef.h>
#include <osg/Array>
#include <osg/Geode>
#include <osg/Vec3d>
#include <vector>


//Forward declarations
namespace utility_functions {
	class SceneKeeper;
}

namespace evolutionary_inspection_plan_evaluation {

//Forward declarations
class PlanCoverageEstimator;
class PlanInterpreterBoxOrder;



class PlanEnergyEvaluator {

private:

	double maxAllowedEnergy;						///The maximum energy we allow any plan to have.
	///The maxAllowedEnergy is only considered if this is true. Useful to set false for instance when we just want to plot
	/// the plan and do not want to spend time calculating a reasonable maximum energy usage
	bool energyLimitValid;

	PlanCoverageEstimator* coverageEstimator;		///A pointer to the object estimating coverage for plans
	PlanInterpreterBoxOrder* planInterpreter;				///The object interpreting our plan (from sequence of numbers to an actual plan)
	const utility_functions::SceneKeeper& sceneKeeper;				///The inspection target


	/**
	 * Calculates the maximum energy any plan is allowed to use. Plans using more are not evaluated, and directly given
	 * the lowest score for coverage. The calculation is based on how much energy a "very comprehensive" plan
	 * would use
	 */
	void calculateMaxAllowedEnergyUsage();







public:

	/**
	 * Constructor - requires pointers to several objects that will be requested for information during energy calculations.
	 * @param collisionPenalty The penalty (an added amout of energy) we give to any edge in a plan that collides with the inspection target.
	 * @param useEnergyLimit If true, we are able to check a plan's energy usage against maximumAllowedEnergy to find out if it is too
	 * long to be considered. If false, this feature is disabled.
	 * @param pe Pointer to the object giving estimates of coverage
	 * @param pi Pointer to the object interpreting plans
	 * @param structure Pointer to the inspection target
	 */
	PlanEnergyEvaluator(bool useEnergyLimit, PlanCoverageEstimator* pe, PlanInterpreterBoxOrder* pi, const utility_functions::SceneKeeper& sceneKeeper)
	:energyLimitValid(useEnergyLimit),
	 coverageEstimator(pe),
	 planInterpreter(pi),
	 sceneKeeper(sceneKeeper){
		if(useEnergyLimit){
			calculateMaxAllowedEnergyUsage(); //Sets maxAllowedEnergy
		}
		else{
			maxAllowedEnergy = -1;
		}
	}

	/**
	 * Checks if the plan uses more energy than we allow.
	 * @param energyUsed The amount of energy the plan uses.
	 * @return true if the plan is within energy bounds OR if energy checking is disabled, otherwise false.
	 */
	bool energyWithinBounds(double energyUsed);


	/**
	 * Decodes a plan and calculates how much energy that plan uses
	 * @param plan The input plan
	 * @return Energy usage of input plan
	 */
	double decodeAndCalculateEnergyUsage(const std::vector<std::vector<double> >& plan);

	double getMaxAllowedEnergy() const {
		return maxAllowedEnergy;
	}

};

} /* namespace evolutionary_inspection_plan_evaluation */

#endif /* PLANENERGYEVALUATOR_H_ */
