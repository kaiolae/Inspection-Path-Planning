/*
 * PlanInterpreterBoxOrder.cpp
 *
 *  Created on: May 8, 2015
 *      Author: kaiolae
 */

#include "PlanInterpreterBoxOrder.h"

#include <iostream>
#include <osg/Polytope>
#include <osg/ShapeDrawable>

#include "../../Utility_Functions/src/Constants.h"
#include "ContourTracing.h"
#include "../../Utility_Functions/src/OsgHelpers.h"
#include "../../Utility_Functions/src/SceneKeeper.h"
#include "../../Utility_Functions/src/HelperMethods.h"


using namespace utility_functions;
namespace evolutionary_inspection_plan_evaluation {

bool PlanInterpreterBoxOrder::boxNeighborsObject(int x, int y, int z) const{
	std::vector<std::vector<int> > currentlayer = boxVolume[z];
	for(int xNeighbor = -1; xNeighbor < 2; xNeighbor++){
		for(int yNeighbor = -1; yNeighbor <2; yNeighbor++){
			if(x+xNeighbor<0||(unsigned)(x+xNeighbor)>=currentlayer[0].size()||
					y+yNeighbor<0||(unsigned)(y+yNeighbor)>=currentlayer.size()){
				continue;
			}
			if(currentlayer[y+yNeighbor][x+xNeighbor]==-1){
				return true;
			}
		}
	}
	return false;
}

void PlanInterpreterBoxOrder::ProduceSimpleCirclingPlans(){

	for(unsigned int z = 0; z<boxVolume.size(); z++){
		std::vector<std::vector<int> > currentlayer = boxVolume[z];

		//Holding true for those boxes that are on the edge or intersecting the structure, false for the others.
		//Thereby, we can later trace around the outer edge of "true" areas, to find edges.
		std::vector<std::vector<bool> > currentLayerEdgeBoxes;
		for(unsigned int y=0; y<currentlayer.size();y++){
			std::vector<int> currentRow = currentlayer[y];
			std::vector<bool> currentRowEdgeBoxes;
			for(unsigned int x=0; x<currentRow.size();x++){
				int currentBox = currentRow[x];
				if(currentBox==-1 || boxNeighborsObject(x,y,z)){
					//Means this waypoint intersects with the scene OR neighbors the scene.
					//In other words, it is "on the edge".
					currentRowEdgeBoxes.push_back(true);
				}
				else{ //The box is NOT a "neighbor of the scene". Not part of simple plan.
					currentRowEdgeBoxes.push_back(false);
				}
			}
			currentLayerEdgeBoxes.push_back(currentRowEdgeBoxes);
		}

		//Tracing around the areas we have found to be on or neighboring the target structure.
		std::vector<std::pair<int,int> > traces = mooreNeighborTracing(&currentLayerEdgeBoxes);
		if(traces.size()==0){
			//If we don't have any plan parts on this level, we move on to the next without adding it to our set of plans
			continue;
		}
		std::vector<std::vector<double> > circlingPlanForCurrentLevel;
		for(unsigned int i =0; i<traces.size();i++){
			int x = traces[i].first;
			int y = traces[i].second;
			//Asserting that the computed plan part is available
			assert(x>=0&&(unsigned)x<currentlayer[0].size()&&y>=0&&(unsigned)y<currentlayer.size());


			//Only adding the necessarry points to the plan. Unnecessarry are those points that are
			//on a straight line between other plan points: The robot will naturally visit these anyway.
			bool prevXIsInPlan = false;
			bool nextXIsInPlan = false;
			bool prevYIsInPlan = false;
			bool nextYIsInPlan = false;
			if(std::find(traces.begin(),traces.end(),std::pair<int,int>(x+1,y))!=traces.end()){
				nextXIsInPlan = true;
			}
			if(std::find(traces.begin(),traces.end(),std::pair<int,int>(x-1,y))!=traces.end()){
				prevXIsInPlan = true;
			}
			if(std::find(traces.begin(),traces.end(),std::pair<int,int>(x,y+1))!=traces.end()){
				nextYIsInPlan = true;
			}
			if(std::find(traces.begin(),traces.end(),std::pair<int,int>(x,y-1))!=traces.end()){
				prevYIsInPlan = true;
			}
			if(!((prevXIsInPlan&& nextXIsInPlan)||(prevYIsInPlan&&nextYIsInPlan))){
				//if we are not on a straight line between other plan points: add the current waypoint.
				std::vector<double> planPart;
				planPart.push_back(boxVolume[z][y][x]);
				circlingPlanForCurrentLevel.push_back(planPart);

			}
		}
		singleLevelCirclingPlans.push_back(circlingPlanForCurrentLevel);

	}

}

std::vector<std::vector<std::vector<double> > > PlanInterpreterBoxOrder::generateOrGetCompleteCirclingPlans(){
	if(completeCirclingPlans.size()>0){
		return completeCirclingPlans;
	}
	int totalNumLevels = singleLevelCirclingPlans.size();


	//Making many different plans, where we "step over" N levels before the next circling.
	for(int stepSize = 1; stepSize<=totalNumLevels;stepSize++){
		std::vector<std::vector<double> > currentPlan;
		std::vector<int> levelsWeWillScan; //The levels we will scan with the current plan
		for(int lvCounter = 0; lvCounter < totalNumLevels; lvCounter+=stepSize){
			levelsWeWillScan.push_back(lvCounter);
		}

		//Adjusting the list of levels so we always focus plans around the middle of the structure.
		int skewDistance;
		int planMidpoint = levelsWeWillScan[levelsWeWillScan.size()/2];
		double middleOfStructure = ((double)totalNumLevels)/2.0;
		//For plans with an odd number of levels, this means skewing such that the middle circuit circles
		//the middle of the structure.
		if(levelsWeWillScan.size()%2==1){//Even number of circuits
			//We want the midpoint to circle the middle part of the structure:
			skewDistance = int(middleOfStructure - planMidpoint);
		}
		else{
			//For plans with an even number of levels, we have to skew by the MEDIAN of the two middle circuit levels.
			//Finding the second midpoint for plans with an even number of circuits.
			int secondMidpoint = levelsWeWillScan[(levelsWeWillScan.size()/2)-1];
			double midpointMedian = double(planMidpoint + secondMidpoint) /2.0;

			skewDistance = int(middleOfStructure - midpointMedian);

		}
		//Adding skew distance to each element of the vector.
		std::transform( levelsWeWillScan.begin(), levelsWeWillScan.end(),
		                levelsWeWillScan.begin(), std::bind2nd( std::plus<int>(), skewDistance ) );


		//Generating the circling plan.
		for(std::vector<int>::iterator it = levelsWeWillScan.begin(); it<levelsWeWillScan.end();it++){
			std::vector<std::vector<double> > singleLvPlan = singleLevelCirclingPlans[*it];
			currentPlan.insert(currentPlan.end(), singleLvPlan.begin(), singleLvPlan.end());
		}
		completeCirclingPlans.push_back(currentPlan);

	}

	return completeCirclingPlans;
}

PlanInterpreterBoxOrder::~PlanInterpreterBoxOrder() {
}

int PlanInterpreterBoxOrder::getNumberOfBoxes() const{
	return boxCenters->size();
}

PlanInterpreterBoxOrder::PlanInterpreterBoxOrder(SceneKeeper& sceneKeeper, osg::ref_ptr<osg::Vec3dArray> boxes, CameraEstimator & camEstimator,
		osg::ref_ptr<osg::Geode> coloredScene, const std::vector<double>* startLocation/* = nullptr*/, bool post_processing/*=false*/)
:startLocation(startLocation),
 camEstimator(camEstimator),
sceneKeeper(sceneKeeper),
coloredScene(coloredScene),
post_processing(post_processing)
{
	boxCenters = new osg::Vec3dArray();
	divideIntoBoxes(boxes);
}

void PlanInterpreterBoxOrder::drawAllBoxes(osg::ref_ptr<osg::Geode> draw_to_geode) const{

	for(auto const& coord: *boxCenters){

		osg::Sphere *wpSphere = new osg::Sphere(coord, 0.25);
		osg::ShapeDrawable *sphereDrawable = new osg::ShapeDrawable(
				wpSphere);
		sphereDrawable->setColor(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
		draw_to_geode->addDrawable(sphereDrawable);
	}
}

void PlanInterpreterBoxOrder::divideIntoBoxes(osg::ref_ptr<osg::Vec3dArray> boxes){

	osg::BoundingBox bb = sceneKeeper.getBoundingBox();

	double paddedBBVolume = ((bb.xMax()-bb.xMin()) + 2*BOX_PADDING) *
			((bb.yMax()-bb.yMin()) + 2*BOX_PADDING) *
			((bb.zMax()-bb.zMin()) + BOX_PADDING);

	double viewpointVolume = paddedBBVolume / NUM_VIEWPOINTS_INSIDE_INSPECTION_VOLUME;
	double viewpoint_interval = std::cbrt(viewpointVolume); //Waypoint interval is the cubed root of the volume around each waypoint.

	double minZ = bb._min.z() + MIN_DIST_FROM_BOTTOM; //To avoid plans colliding with ocean floor.
	double maxZ = bb._max.z();
	if(DOWNWARD_CAMERA_ACTIVE){
		maxZ+=BOX_PADDING; //To allow plans going above structures, to inspect them.
	}
	for(double z = minZ; z<maxZ; z+=viewpoint_interval){
		std::vector<std::vector<int> > currentLayer;
		for(double x = bb._min.x()-BOX_PADDING; x<bb._max.x()+BOX_PADDING; x+=viewpoint_interval){
			std::vector<int> currentRow;
			for(double y = bb._min.y()-BOX_PADDING; y<bb._max.y()+BOX_PADDING; y+=viewpoint_interval){
				osg::Vec3d viewpoint(x,y,z);
				//Box used to check if viewpoints have sufficient distance from the target.
				osg::Polytope distanceBox = generateBoxPolytope(viewpoint,BOX_SAFETY_MARGIN);
				bool viewpointIsTooClose = checkPolytopeForIntersections(distanceBox,*sceneKeeper.getScene());
				if(viewpointIsTooClose){
					currentRow.push_back(-1);
					continue;
				}

				//Box used to check if viewpoints are close enough to see the target
				//TODO: This would be better approximated by a sphere, but OSG does not have a class for
				//sphere intersections. Given that we don't need a 100% accuracy here, I don't know if implementing that is worth the cost.
				//On second thought, a box is not so bad here - the camera does cover a rectangle at CAMERA_FAR_PLANE_DIST.
				//Anyway, it should not be a problem that we overestimate here, as it will in the worst case lead us to
				//consider some unnecessarry viewpoints in our planner. But visibility calculations will correctly tell us
				//that we cannot see anything at this viewpoint.
				osg::Polytope visibilityBox = generateBoxPolytope(viewpoint, CAMERA_FAR_PLANE_DIST);
				bool anythingIsVisible = checkPolytopeForIntersections(visibilityBox,*sceneKeeper.getScene());

				//bool anythingIsVisible = viewpointIsUseful(viewpoint); //Checks if we can see anything at all from that point. Otherwise, we don't consider it as a candidate viewpoint.
				if(anythingIsVisible){
					currentRow.push_back(boxCenters->size());

					boxCenters->push_back(osg::Vec3d(x,y,z));
					if(boxes!=nullptr){
						boxes->push_back(osg::Vec3d(x,y,z));
					}
				}
				else{//If nothing was visible here, we do not consider a waypoint.
					currentRow.push_back(-2);
				}
			}//Iteration over current row done
			currentLayer.push_back(currentRow);
		}//Iteration over current layer(z-value) done
		boxVolume.push_back(currentLayer);

	}
}

osg::Vec3d PlanInterpreterBoxOrder::getPositionFromGene(const std::vector<std::vector<double> >& genotype, size_t geneID) const{
	size_t pointID = (size_t) (genotype[geneID][0]+0.5);
	return getBoxPosition(pointID);
}

osg::Vec3d PlanInterpreterBoxOrder::calculateSensorDirectionAndNormalize(const osg::Vec3d& prevPosition, const osg::Vec3d& nextPosition, float offsetRadians/*=0*/) const{
	//Calculating the direction the sensor will have when moving from previous to current point.
	osg::Vec3d sensorDirection;
	if(post_processing){
		//Slow calculation - for post-processing
		sensorDirection = calculateViewingDirectionTowardsPrimitives(prevPosition,nextPosition,sceneKeeper.getSceneCenter(),
																	 this->camEstimator, coloredScene,sceneKeeper.getTriangleStore());
	}
	else{
		//Quick calculation - during optimization
		sensorDirection = calculateViewingDirection(prevPosition,nextPosition,sceneKeeper.getSceneCenter());
	}
	sensorDirection.normalize();
	double vectorRadians = atan2(sensorDirection.y(),sensorDirection.x());//acos(sensorDirection*osg::Vec3d(1,0,0));
	vectorRadians+=offsetRadians;
	sensorDirection = osg::Vec3d(cos(vectorRadians), sin(vectorRadians), 0);
	sensorDirection.normalize();
	return sensorDirection;
}

void PlanInterpreterBoxOrder::InterpretOnePlanPoint(osg::Vec3d* prevPosition, const std::vector<double>& encodedPlanPoint, osg::Vec3dArray& positions,
		osg::Vec3dArray& sensorDirections) const{

	size_t pointID = (size_t) (encodedPlanPoint[0]+0.5);
	assert(pointID < boxCenters->size());
	float sensor_dir_offset = 0;
	if (encodedPlanPoint.size() > 1){
		sensor_dir_offset = encodedPlanPoint[1];
	}

	osg::Vec3d currentPosition = boxCenters->at(pointID);

	positions.push_back(currentPosition); //Rounding to nearest int, in case the Python-C type translation has turned the int into a double.
	//Calculating the direction the sensor will have when moving from previous to current point.
	//If the current point is the first of the plan, this is naturally omitted.
	if(prevPosition!=nullptr){
		osg::Vec3d sensorDirection = calculateSensorDirectionAndNormalize(*prevPosition, currentPosition, sensor_dir_offset);
		sensorDirections.push_back(sensorDirection);
	}


}

void PlanInterpreterBoxOrder::InterpretPlan(const std::vector<std::vector<double> >& plan, osg::Vec3dArray& plannedPositions, osg::Vec3dArray& plannedAngles) const{
	if(plan.size()==0){
		return;
	}

	if(startLocation!=nullptr){ //If given, the first point is equal to the startlocation.
		plannedPositions.push_back(osg::Vec3d((*startLocation)[0], (*startLocation)[1], (*startLocation)[2]));
	}

	int counter=0;
	for(std::vector<std::vector<double> >::const_iterator it = plan.begin(); it!=plan.end();it++){
		std::vector<double> nextPlanStep = *it;
		if(plannedPositions.size()!=0){ //True for all but the first point in the plan.
			osg::Vec3d previousPoint = plannedPositions.back();
			InterpretOnePlanPoint(&previousPoint,nextPlanStep,plannedPositions,plannedAngles);
		}
		else{
			//For the first plan part, previous step is "nullptr".
			InterpretOnePlanPoint(nullptr,nextPlanStep,plannedPositions,plannedAngles);
		}

		counter++;
	}
}


osg::Vec3d PlanInterpreterBoxOrder::getBoxPosition(size_t id) const{
	assert(id<boxCenters->size());
	return boxCenters->at(id);
}


} /* namespace evolutionary_inspection_plan_evaluation */

