/*
 * OsgHelpers.cpp
 *
 *  Created on: Jul 31, 2015
 *      Author: kaiolae
 */

#include "OsgHelpers.h"

#include <assert.h>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/ShapeDrawable>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/PolytopeIntersector>
#include <osgText/Text>
#include <osgViewer/Viewer>
#include <osg/LightModel>
#include <osgGA/OrbitManipulator>

#include "Constants.h"
#include "CameraEstimator.h"
#include "HelperMethods.h"
#include "SceneKeeper.h"
#include "KeyboardInputHandler.h"



namespace utility_functions {

	///Used from the Python interface.
	std::vector<std::vector<double> > viewMatrixSelector(
			const std::string& sceneFileName) {
        std::cout << "Viewmatrix selector for file " << sceneFileName << std::endl;
		osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(sceneFileName);
        if (!scene)
        {
            throw std::invalid_argument( "The following file does not specify a valid 3D model for OpenSceneGraph: " + sceneFileName );
        }
		osgViewer::Viewer viewer;
		osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
		lightModel->setTwoSided(true);
		scene->getOrCreateStateSet()->setAttributeAndModes(
				lightModel.get());
		viewer.setSceneData(scene.get());

		std::cout << "Press w to store camera location." << std::endl;
		viewMatrixStorage* vmStorage = new viewMatrixStorage;
		KeyboardInputHandler* myFirstEventHandler = new KeyboardInputHandler(
				vmStorage);
		viewer.addEventHandler(myFirstEventHandler);

		viewer.run();
		osg::Matrixd mat = vmStorage->getMostRecentViewMatrix();

		return convertMatrixToNestedVector(mat);
	}

	void display_points_and_edges(const std::vector<std::vector<double> >* points/*=nullptr*/,
                                  const std::vector<std::pair<std::vector<double>, std::vector<double> > >* edges/*=nullptr*/,
                                  osg::ref_ptr<osg::Geode> g/*=nullptr*/){
        bool g_not_given = false;
        if(g== nullptr){
            g_not_given = true;
            g=new osg::Geode();
        }

        std::cout << "Points is " << points << std::endl;
        if(points!=nullptr) {
            std::cout << "Points size: " << points->size() << std::endl;
            for (const auto &point: *points) {
                osg::Vec3d osg_point(point[0], point[1], point[2]);
                std::vector<double> v = point;
                std::cout << "Adding point " << v << std::endl;
                g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg_point, 0.4)));
            }
        }

        if(edges!=nullptr) {
            for (const auto &edge: *edges) {
                osg::Vec3 start(edge.first[0], edge.first[1], edge.first[2]);
                osg::Vec3 end(edge.first[0], edge.first[1], edge.first[2]);
                osg::ref_ptr<osg::Geometry> beam(new osg::Geometry);
                osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array;

                points->push_back(start);
                points->push_back(end);
                beam->setVertexArray(points.get());
                beam->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));
                g->addDrawable(beam);
            }
        }

        std::cout << "Display. g is " << g << std::endl;
        if(g_not_given){
            osgViewer::Viewer viewer;
            viewer.setSceneData(g);
            viewer.run();
        }

	}

//Calculates the energy used by a plan.
	double calculateEnergyUsageOfPlan(const osg::Vec3dArray &plannedPositions, const osg::ref_ptr<osg::Node> potential_colliders/*=nullptr*/,
                                          double collision_penalty/*=0*/) {
	std::vector<std::vector<double> > tempVector = convertOsgVec3dArrayToVector(plannedPositions);
	if(plannedPositions.size()==0){
		return 0;
	}
	double energyUsage = 0;
	osg::Vec3d currentLocation = plannedPositions[0];
	int counter = 0;
	for(osg::Vec3dArray::const_iterator posItt = plannedPositions.begin(); posItt<plannedPositions.end(); posItt++){
		if(counter!=0){
			osg::Vec3d nextLocation = *posItt;
			if(counter > 1){ //We only calculate the turning energy from the second edge, as we don't have to turn before the first edge.
				energyUsage += calculateEnergyUsageOneMove(currentLocation, nextLocation, potential_colliders, collision_penalty,
														   &plannedPositions[counter - 2]);
			}
			else{
				energyUsage += calculateEnergyUsageOneMove(currentLocation, nextLocation, potential_colliders, collision_penalty);
			}
			currentLocation = nextLocation;
		}
		counter ++;
	}
	return energyUsage;
}

double calculateEnergyUsageOneMove(const osg::Vec3d &start, const osg::Vec3d &end,
								   const osg::ref_ptr<osg::Node> potential_colliders/*=nullptr*/,
								   double collision_penalty/* = 0*/,
								   const osg::Vec3d *previousPoint/*=nullptr*/) {
	if(start==end){
		return 0.0;
	}
	//The energy related to the actual vehicle translation.
	double energyUsage = translationEnergyFactor*((end-start).length());
	//The energy related to rotation (large, because it requires speed changes).
	if(previousPoint!=nullptr){ ///<If this is the first translation, we didn't use any energy to rotate.
		assert(*previousPoint!=start); ///<This is not supposed to happen, and leads to erroneous calculations.
		osg::Vec3d pp = *previousPoint;
		osg::Vec3d incomingVector = start-pp;
		osg::Vec3d outgoingVector = end-start;

		///Angle calculation
		double angleCos = (incomingVector*outgoingVector)/(incomingVector.length()*outgoingVector.length());
		//angleCos of -1 implies the largest possible turn (opposite direction), 0 implies 90 degree turn, and 1 implies no turn (continue straight).
		double turnMagnitude = 1.0-angleCos; // Therefore, this magnitude will be 2 for a 180 turn, 1 for a 90 degree, and 0 for no turn.
		energyUsage += turnMagnitude*rotationEnergyFactor;
	}

	// Energy calculation disregarding collisions.
	if(potential_colliders == nullptr || collision_penalty<=0){
		return energyUsage;
	}
	// Energy calculation including collision checks.
	if(edgeComesTooCloseToStructure(start,end,*potential_colliders)){
		energyUsage += collision_penalty;
	}
	return energyUsage;
}

std::vector<std::vector<double> > convertOsgVec3dArrayToVector(const osg::Vec3dArray& osgArray){
	std::vector<std::vector<double> > stdVector(osgArray.size(),std::vector<double>(3,0));
	for(size_t row = 0; row<osgArray.size(); row++){
		for(size_t col = 0; col<3; col++){
			stdVector[row][col] = osgArray[row][col];
		}
	}
	return stdVector;
}

std::vector<double> convertOsgVectorToVector(const osg::Vec3d& vector){
	std::vector<double> cVector;
	cVector.push_back(vector.x());
	cVector.push_back(vector.y());
	cVector.push_back(vector.z());

	return cVector;
}



/**
 * Generates a polytope encompassing an edge in the AUV's plan. The generated volume represents the "safety volume" that we can assume the AUV stays inside during
 * translation, and is therefore used to decide whether or not the edge contains a collision.
 * This is a simplified collision check that strikes a balance between efficiency and precision.
 * @param edgeStart The start position of the edge
 * @param edgeEnd The end position of the edge
 * @param g Optional drawable. If given, we can plot the edge location to it.
 * @return A box-shaped polytope, with the edge in its center and a safety buffer around the edge on each side.
 */
osg::Polytope generatePolytopeFromPlanEdge(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd,  osg::ref_ptr<osg::Geode> g){

	//The direction vectors we need
	osg::Vec3d edgeDirection = (edgeEnd - edgeStart);
	edgeDirection.normalize();
	osg::Vec3d upDirection = UP_VECTOR;
	if(edgeEnd.x()==edgeStart.x() && edgeEnd.y()==edgeStart.y()){
		//The special case of a waypoint that is directly above another
		upDirection = osg::Vec3d(1,0,0);
	}
	osg::Vec3d rightDirection = edgeDirection^upDirection;

	//Points on the polytope we are about to create.
	osg::Vec3 startLowerLeft(edgeStart-(upDirection*EDGE_SAFETY_BUFFER)-(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 startLowerRight(edgeStart-(upDirection*EDGE_SAFETY_BUFFER)+(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 startUpperLeft(edgeStart+(upDirection*EDGE_SAFETY_BUFFER)-(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 startUpperRight(edgeStart+(upDirection*EDGE_SAFETY_BUFFER)+(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 endLowerLeft(edgeEnd-(upDirection*EDGE_SAFETY_BUFFER)-(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 endLowerRight(edgeEnd-(upDirection*EDGE_SAFETY_BUFFER)+(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 endUpperLeft(edgeEnd+(upDirection*EDGE_SAFETY_BUFFER)-(rightDirection*EDGE_SAFETY_BUFFER));
	osg::Vec3 endUpperRight(edgeEnd+(upDirection*EDGE_SAFETY_BUFFER)+(rightDirection*EDGE_SAFETY_BUFFER));
	//The planes of the frustum
	osg::Plane farPlane = getPlane(endLowerLeft, endLowerRight, endUpperRight,-edgeDirection);
	osg::Plane nearPlane = getPlane(startLowerRight, startLowerLeft, startUpperLeft,edgeDirection);
	osg::Plane leftPlane = getPlane(startLowerLeft,startUpperLeft, endUpperLeft,rightDirection);
	osg::Plane rightPlane = getPlane(startLowerRight, startUpperRight, endUpperRight,-rightDirection);
	osg::Plane bottomPlane = getPlane(startLowerRight, startLowerLeft, endLowerLeft, upDirection);
	osg::Plane topPlane = getPlane(startUpperLeft, startUpperRight, endUpperRight,-upDirection);

	osg::Polytope p;
	p.add(farPlane);
	p.add(nearPlane);
	p.add(leftPlane);
	p.add(rightPlane);
	p.add(topPlane);
	p.add(bottomPlane);

	if(g!=nullptr && edgeEnd.x()==edgeStart.x() && edgeEnd.y()==edgeStart.y()){
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(startLowerLeft, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(startUpperLeft, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(startLowerRight, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(startUpperRight, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(endLowerLeft, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(endUpperLeft, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(endLowerRight, 0.2)));
		g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(endUpperRight, 0.2)));
	}

	return p;
}

bool edgeComesTooCloseToStructure(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd, osg::Node& structure, const osg::ref_ptr<osg::Geode> g/*=nullptr*/){
	osg::Polytope edgeWithBuffer = generatePolytopeFromPlanEdge(edgeStart, edgeEnd,g); //A box representing the edge with safety margin.

	osgUtil::PolytopeIntersector* polytopeIntersector = new osgUtil::PolytopeIntersector(edgeWithBuffer);
	osgUtil::IntersectionVisitor intersectVisitor( polytopeIntersector);
	structure.accept(intersectVisitor);
	if(polytopeIntersector->containsIntersections()){
		if(g!=nullptr){
			osgUtil::PolytopeIntersector::Intersections intersections = polytopeIntersector->getIntersections();
			osgUtil::PolytopeIntersector::Intersections::iterator itt = intersections.begin();
			osg::Vec3d polyIntersect = itt->localIntersectionPoint;
			g->addDrawable(new osg::ShapeDrawable(new osg::Sphere(polyIntersect, 0.2)));
		}
		return true;
	}
	return false;
}


osg::ref_ptr<osg::FloatArray> convertOsgVectorToArray(osg::Vec3d vector){
	osg::ref_ptr<osg::FloatArray> array = new osg::FloatArray;
	array->push_back(vector.x());
	array->push_back(vector.y());
	array->push_back(vector.z());
	return array;
}

osg::ref_ptr<osg::Vec3dArray> convert2dVectorToOsgVec3dArray(const std::vector<std::vector<double> >& vec){
	osg::ref_ptr<osg::Vec3dArray> array = new osg::Vec3dArray;
	for(std::vector<std::vector<double> >::const_iterator vectorItt = vec.begin(); vectorItt != vec.end(); vectorItt++){
		std::vector<double> currentPoint = *vectorItt;
		osg::Vec3d currentPointOsg(currentPoint[0],currentPoint[1],currentPoint[2]);
		array->push_back(currentPointOsg);
	}
	return array;
}

//Produces a plane from the three point, with a normal pointing in vectorDirection.
osg::Plane getPlane(const osg::Vec3d& point1, const osg::Vec3d&  point2, const osg::Vec3d&  point3, const osg::Vec3d&  vectorDirection){
	osg::Plane resultPlane;
	resultPlane.set(point1, point2, point3);
	if(resultPlane.dotProductNormal(vectorDirection) < 0){
		//Flips the plane if the frustum contents are not "within" the frustum.
		resultPlane.flip();
	}
	return resultPlane;
}


osg::Polytope generateBoxPolytope(const osg::Vec3d& center, double side_length){

	osg::Vec3d rightVector = osg::Vec3d(0,1,0);
	osg::Vec3d forwardVector = osg::Vec3d(1,0,0);
	osg::Vec3d upVector = osg::Vec3d(0,0,1);

	//The planes of the frustum
	osg::Plane farPlane = getPlane(center+(forwardVector*side_length), center+(forwardVector*side_length)+rightVector, center+(forwardVector*side_length)+upVector,-(forwardVector*side_length));
	osg::Plane nearPlane = getPlane(center-(forwardVector*side_length), center-(forwardVector*side_length)+rightVector, center-(forwardVector*side_length)+upVector,(forwardVector*side_length));
	osg::Plane leftPlane = getPlane(center-(rightVector*side_length), center+forwardVector-(rightVector*side_length), center+upVector-(rightVector*side_length),(rightVector*side_length));
	osg::Plane rightPlane = getPlane(center+(rightVector*side_length), center+forwardVector+(rightVector*side_length), center+upVector+(rightVector*side_length),-(rightVector*side_length));
	osg::Plane bottomPlane = getPlane(center-(upVector*side_length), center+forwardVector-(upVector*side_length), center-(upVector*side_length)-rightVector,(upVector*side_length));
	osg::Plane topPlane = getPlane(center+(upVector*side_length), center+forwardVector+(upVector*side_length), center+(upVector*side_length)-rightVector,-(upVector*side_length));

	osg::Polytope frustum;
	frustum.add(farPlane);
	frustum.add(nearPlane);
	frustum.add(leftPlane);
	frustum.add(rightPlane);
	frustum.add(topPlane);
	frustum.add(bottomPlane);

	return frustum;
}


//Checks the given polytope for intersections with the given scene.
bool checkPolytopeForIntersections(const osg::Polytope& polytope, osg::Node& scene){
	osgUtil::PolytopeIntersector* polytopeIntersector = new osgUtil::PolytopeIntersector(polytope);
	osgUtil::IntersectionVisitor intersectVisitor( polytopeIntersector);
	scene.accept(intersectVisitor);
	if(polytopeIntersector->containsIntersections()){
		return true;
	}
	else{
		return false;
	}
}

bool collisionCheckWithBuffer(const osg::Vec3d& point, osg::Node& structure, double bufferRadius){
	osg::Polytope distanceBox = generateBoxPolytope(point,bufferRadius);
	return checkPolytopeForIntersections(distanceBox,structure);

}

void draw_viewpoint(const osg::Vec3d& point, const osg::Vec3d& view_direction, osg::ref_ptr<osg::Geode> geode){
		osg::ref_ptr<osg::Sphere> nodeSphere = new osg::Sphere(point, 0.25);
		osg::Vec3d viewingDirection = point + view_direction;
		osg::ref_ptr<osg::Sphere> viewSphere = new osg::Sphere(viewingDirection, 0.12);
		osg::ref_ptr<osg::ShapeDrawable> sphereDrawable = new osg::ShapeDrawable(nodeSphere);
		osg::ref_ptr<osg::ShapeDrawable> sphereDrawable2 = new osg::ShapeDrawable(viewSphere);
		//sphereDrawable->setColor( osg::Vec4(0.5f,0.5f,0.5f,1.0f));
		geode->addDrawable(sphereDrawable);
		geode->addDrawable(sphereDrawable2);
}

void drawTrajectory(osg::Vec3dArray* planPositions, const osg::Vec3dArray* angles, osg::Geode& geode){
	if (planPositions->size() < 2){
		//If we don't have at least 2 positions, we cannot draw a line.
		return;
	}

	osg::ref_ptr<osg::Geometry> trajectory = osg::ref_ptr<osg::Geometry>(new osg::Geometry());
	trajectory->setVertexArray(planPositions);

	osg::PrimitiveSet *prset = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, planPositions->size());
	trajectory->addPrimitiveSet(prset);
	osg::LineWidth *linewidth = new osg::LineWidth();
	linewidth->setWidth(7.0f);

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(osg::Vec4d(0.0,0.0,0.0,1.0));
	trajectory->setColorArray(colors.get());
	trajectory->setColorBinding(osg::Geometry::BIND_OVERALL);

	osg::StateSet *stateset = new osg::StateSet;
	stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	geode.setStateSet(stateset);

	//Visualizing the viewpoints
	for (auto const& waypoint : *planPositions) {
		osg::ref_ptr<osg::ShapeDrawable> wp_sphere = new osg::ShapeDrawable(new osg::Sphere(waypoint, 0.1));
		wp_sphere->setColor(osg::Vec4d(0.0,0.0,0.0,1.0));
		geode.addDrawable(wp_sphere);
	}

	geode.addDrawable(trajectory);


}


osg::Vec3d calculateTriangleNormal(const osg::Vec3d& corner1, const osg::Vec3d& corner2, const osg::Vec3d& corner3){

	osg::Vec3d side1 = corner2-corner1;
	osg::Vec3d side2 = corner3-corner1;

	osg::Vec3d surfaceNormal = side1^side2;
	surfaceNormal.normalize();

	return surfaceNormal;
}

std::vector<std::vector<double> > convertMatrixToNestedVector(const osg::Matrixd& mat){
	std::vector<std::vector<double> > vec;
	for (int i = 0; i < 4; ++i)
	{
		std::vector<double> row;
		for(int j= 0; j<4; j++){
		  row.push_back(mat(i,j));
		}
		vec.push_back(row);
	}
	return vec;
}

osg::Matrixd convertNestedVectorToMatrix(const std::vector<std::vector<double> >& vec){
	osg::Matrixd mat(vec[0][0],vec[0][1],vec[0][2],vec[0][3],
			vec[1][0],vec[1][1],vec[1][2],vec[1][3],
			vec[2][0],vec[2][1],vec[2][2],vec[2][3],
			vec[3][0],vec[3][1],vec[3][2],vec[3][3]);

	return mat;
}

osg::Vec3d calculateViewingDirection(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd, const osg::Vec3d& sceneCenter, bool perpendicularViewing /*=true*/){
	osg::Vec3d sensorDirection;
	//Calculating default sensor direction. Optimized values (if any) are added as offset to this direction.
	if(!perpendicularViewing||(edgeStart.x()==edgeEnd.x()&&edgeStart.y()==edgeEnd.y())){
		//For movements purely in z-direction: Look towards the center of the scene.
		sensorDirection = sceneCenter - ((edgeEnd+edgeStart)/2.0); //Looking towards structure from midpoint of the movement.
		sensorDirection[2] = 0; //Always looking straight ahead in z-dir, as we don't want to tilt the robot around that axis.
	}
	else{ //For movements in x- and/or y-direction, sensor direction is perpendicular
		//to a plane containing the movement direction and the "up-direction".
		//In other words, the looking direction is perpendicular to the movement direction.
		sensorDirection = UP_VECTOR^(edgeEnd-edgeStart);
		//In addition, the sensor has to point "towards" the inspection target. This is evaluated by checking if it looks "towards"
		//the center of the scene. If not, it is flipped.
		osg::Vec3d midpoint = edgeEnd + (edgeEnd-edgeStart)/2.0;
		osg::Vec3d wantedViewingDirection = (sceneCenter-midpoint);
		wantedViewingDirection.z() = 0; //We want to look straight ahead in the z-plane.

		if(sensorDirection*wantedViewingDirection<0){
			//A negative dot product means we are looking away from the center of the scene, so we flip the sensor direction.
			sensorDirection=-sensorDirection;
		}
	}

	return sensorDirection;
}


//Calculates the viewing direction exactly like the method above, except instead of flipping the view towards the scene center,
//it is flipped in the direction of most primitives.
osg::Vec3d calculateViewingDirectionTowardsPrimitives(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd, const osg::Vec3d& sceneCenter,
		CameraEstimator& camEstimator, osg::ref_ptr<osg::Node> coloredInspectionTarget, const TriangleData& td, bool perpendicularViewing /*=true*/){
	osg::Vec3d sensorDirection;
	//Calculating default sensor direction. Optimized values (if any) are added as offset to this direction.
	if(!perpendicularViewing||(edgeStart.x()==edgeEnd.x()&&edgeStart.y()==edgeEnd.y())){
		//For movements purely in z-direction: Look towards the center of the scene.
		sensorDirection = sceneCenter - ((edgeEnd+edgeStart)/2.0); //Looking towards structure from midpoint of the movement.
		sensorDirection[2] = 0; //Always looking straight ahead in z-dir, as we don't want to tilt the robot around that axis.
	}
	else{
		//For movements in x- and/or y-direction, sensor direction is perpendicular
		//to a plane containing the movement direction and the "up-direction".
		//In other words, the looking direction is perpendicular to the movement direction.
		sensorDirection = UP_VECTOR^(edgeEnd-edgeStart);
	}

	osg::Vec3d reverseSensorDirection = -sensorDirection;

	//Checking which of the two candidate directions gathers the most primitives.
	boost::dynamic_bitset<> colorsDirection1(td.getTriangleCount());
	boost::dynamic_bitset<> colorsDirection2(td.getTriangleCount());

	camEstimator.GetColorsDuringTraversal(edgeStart, edgeEnd, sensorDirection, coloredInspectionTarget, colorsDirection1);
	double coverage1 = td.calculateAndColorCoverage(colorsDirection1);
	camEstimator.GetColorsDuringTraversal(edgeStart, edgeEnd, reverseSensorDirection, coloredInspectionTarget, colorsDirection2);
	double coverage2 = td.calculateAndColorCoverage(colorsDirection2);

	//Higher coverage score means worse performance (optimum is 0). Returning the viewing direction with the lowest coverage.
	if(coverage2==coverage1){
		//If the directions have the same coverage, we default to the standard way of calculating the direction.
		return calculateViewingDirection(edgeStart, edgeEnd, sceneCenter,perpendicularViewing);
	}
	else if(coverage2>coverage1){
		return sensorDirection;
	}
	else{
		return reverseSensorDirection;
	}
}

osg::BoundingBox& calculateBoundingBox(osg::ref_ptr<osg::Node> scene){
	osg::ComputeBoundsVisitor cbv;
	osg::BoundingBox &bb(cbv.getBoundingBox());
	scene->accept(cbv);

	return bb;
}

double calcLongestSceneSide(osg::ref_ptr<osg::Node> scene){

	osg::ComputeBoundsVisitor cbv;
	osg::BoundingBox &bb(cbv.getBoundingBox());
	scene->accept(cbv);

	double x_len = fabs(bb._max.x()-bb._min.x());
	double y_len = fabs(bb._max.y()-bb._min.y());
	double z_len = fabs(bb._max.z()-bb._min.z());


	return std::max(z_len,std::max(x_len,y_len));
}

	void run_rotating_camera(osg::ref_ptr<osg::Node> center_node, int orbit_distance/*=100.0*/, double orbit_angle/*=45.0*/){
		osg::Group* root = NULL;
		osgViewer::Viewer viewer;

		root = new osg::Group();
		root->addChild(center_node);

		osgGA::OrbitManipulator *manipulator = new osgGA::OrbitManipulator();
		viewer.setCameraManipulator(manipulator);
		viewer.setSceneData( root );
		viewer.realize();

		osg::Quat quat;

		double heading = 0;
		manipulator->setDistance(100); //Distance to the object we are orbiting

		//The base angle to look at the structure while rotating. The "camera tilt"
		osg::Matrixd base_rotation = osg::Matrixd::rotate(45.0f, osg::Vec3f(1.0f, 0.0f, 0.0f));

		while( !viewer.done() )
		{
			heading+=0.015;
			osg::Matrixd rotation_matrix = osg::Matrixd::rotate(heading, osg::Vec3(0.0, 0.0, 1.0));
			osg::Matrixd full_rotation = base_rotation*rotation_matrix;
			quat.set(full_rotation);
			manipulator->setRotation(quat);
			viewer.frame();
		}
	}



} /* namespace utility_functions */
