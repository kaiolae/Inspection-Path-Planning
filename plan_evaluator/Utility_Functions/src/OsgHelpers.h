/*
 * OsgHelpers.h
 *
 *  Created on: Jul 31, 2015
 *      Author: kaiolae
 */

#ifndef OSGHELPERS_H_
#define OSGHELPERS_H_

#include <stddef.h>
#include <osg/Array>
#include <osg/BoundingBox>
#include <osg/Geode>
#include <osg/Matrixd>
#include <osg/Plane>
#include <osg/Polytope>
#include <osg/ref_ptr>
#include <osg/Vec3d>
#include <osgDB/Serializer>
#include <vector>
#include <fstream>



namespace utility_functions {



	//Forward declarations
	class CameraEstimator;
	class SceneKeeper;
	class TriangleData;

	std::vector<std::vector<double> > viewMatrixSelector(const std::string& sceneFileName);


/**
 * Estimates the energy usage of a plan, by considering the total distance traversed, the number and magnitude of turns, as well as (optionally) any collisions.
 * @param plannedPositions All the positions in the plan we are evaluating.
 * @param potential_colliders (optional) An OSG node containing any 3D structures the plan may collide with (typically the inspection target).
 * @param collision_penalty (optional) The penalty for colliding with any object in potential_colliders.
 * @return A number representing the energy usage
 */
	double calculateEnergyUsageOfPlan(const osg::Vec3dArray &plannedPositions, const osg::ref_ptr<osg::Node> potential_colliders = nullptr,
                                          double collision_penalty = 0);

/**
 * Estimates the energy used to move from one point to another.
 * @param start The coordinate of the start position
 * @param end The coordinate of the end position
 * @param potential_colliders (optional) An OSG node containing any 3D structures the plan may collide with (typically the inspection target).
 * @param collision_penalty (optional) The penalty for colliding with any object in potential_colliders.
 * @param previousPoint The point we visited BEFORE start. If this is given, we add to the translation energy the rotation energy that needs to be applied in start
 * in order to change the vehicle's direction to go towards end.
 * @return A number representing the energy usage
 */
	double calculateEnergyUsageOneMove(const osg::Vec3d &start, const osg::Vec3d &end,
                                           const osg::ref_ptr<osg::Node> potential_colliders = nullptr,
                                           double collision_penalty = 0,
									   		const osg::Vec3d *previousPoint = nullptr);



/**
 * This is a simplified collision check for edges. We want to avoid a real collision check, for performance reasons, and since we only need a rough estimate
 * at this point: Actual collision detection and avoidance will of course be carried out by other parts of the AUV.
 * The method checks if an edge in the plan comes too close to the structure, by testing if any parts of the structure are inside a box surrounding the edge.
 * @param edgeStart The starting coordinate of the edge
 * @param edgeEnd The end coordinate of the edge
 * @param structure The inspection target
 * @param g Optional. If given, plot to this geode.
 * @return true if we "collide", false otherwise.
 */
bool edgeComesTooCloseToStructure(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd, osg::Node& structure, const osg::ref_ptr<osg::Geode> g=nullptr);

/**
 * Checks if a given xyz-position is in collision with the inspection target. A buffer can be given, if we want to check if we are no
 * closer than N meters from the target.
 * @param point The point we want to check for collision
 * @param structure The structure we want to check for collisions with
 * @param bufferRadius The closest we can be to the target without being in collision.
 * @return True the position is in collision, false otherwise.
 */
bool collisionCheckWithBuffer(const osg::Vec3d& point, osg::Node& structure, double bufferRadius);

/**
 * Creates a plane, as uniquely defined by three different points and a normal direction.
 * @param point1 A point in the plane
 * @param point2 A point in the plane
 * @param point3 A point in the plane
 * @param vectorDirection The direction we want the resulting plane's normal to point
 * @return The produced plane
 */
osg::Plane getPlane(const osg::Vec3d&  point1, const osg::Vec3d&  point2, const osg::Vec3d&  point3, const osg::Vec3d&  vectorDirection);

/**
 * Generates a box volume around a given point.
 * @param center The point we want to generate a box around
 * @param half_side_length Half the side length of the box. That is, the length the box extends from the center in each direction.
 * @return A box volume
 */
osg::Polytope generateBoxPolytope(const osg::Vec3d& center, double half_side_length);

/**
 * Checks the given polytope for intersections with the given scene.
 * @param polytope A volume we want to check for intersections
 * @param scene The scene we want to check if intersects with our volume
 * @return True if there is an intersection, false otherwise.
 */
bool checkPolytopeForIntersections(const osg::Polytope& polytope, osg::Node& scene);

/*
 * Draws a single viewpoint as a large sphere (the point) with a smaller sphere pointing in the viewing direction.
 */
void draw_viewpoint(const osg::Vec3d& point, const osg::Vec3d& view_direction, osg::ref_ptr<osg::Geode> geode);

/**
 * Visualizes a plan, given as the points the AUV will visit. Viewing angles are currently not
 * plotted, to avoid a messy plot.
 * @param planPositions List of all the coordinates the AUV will visit, in the order they will be visited.
 * @param angles List of the angle the AUV will have while traversing each edge in the plan.
 * @param g A geode that the trajectory will be plotted to.
 */
void drawTrajectory(osg::Vec3dArray* planPositions, const osg::Vec3dArray* angles, osg::Geode& geode);

/**
 * Calculates the normal of a given triangle
 * @param corner1, corner 2, corner 3: The three different corners of the triangle
 * @return The triangle's normal
 */
osg::Vec3d calculateTriangleNormal(const osg::Vec3d& corner1, const osg::Vec3d& corner2, const osg::Vec3d& corner3);


osg::Matrixd convertNestedVectorToMatrix(const std::vector<std::vector<double> >& vec);

std::vector<std::vector<double> > convertMatrixToNestedVector(const osg::Matrixd& mat);

std::vector<double> convertOsgVectorToVector(const osg::Vec3d& vector);

std::vector<std::vector<double> > convertOsgVec3dArrayToVector(const osg::Vec3dArray& osgArray);


/**
 * Calculates the direction perpendicular to a given edge, with the additional constraint of looking "towards" the center of the scene.
 * More specifically, the calculated viewing direction always looks straight ahead in the z-direction, while being perpendicular to the movement
 * in the x-y plane - as well as looking more in the direction of the scene center than in the oppsite direction.
 * @param edgeStart The start position of an edge in the plan
 * @param edgeEnd The end position of an edge in the plan
 * @param sceneCenter The coordinates of the center of the inspection target
 * @param perpendicularViewing If true, viewing direction is "straight ahead", perpendicular to robot path. This makes sense for large sweeps.
 * If false, viewing direction is towards the scene center. This makes sense for up-close inspection of details.
 * @return The returned direction vector, representing the AUV's default viewing direction.
 */
osg::Vec3d calculateViewingDirection(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd, const osg::Vec3d& sceneCenter, bool perpendicularViewing =true);

osg::Vec3d calculateViewingDirectionTowardsPrimitives(const osg::Vec3d& edgeStart, const osg::Vec3d& edgeEnd, const osg::Vec3d& sceneCenter,
		CameraEstimator& camEstimator, osg::ref_ptr<osg::Node> coloredInspectionTarget, const TriangleData& td, bool perpendicularViewing =true);

/**
 * Calculates the bounding box of a scene
 * @param scene A 3D model we want the bounding box of.
 * @return Bounding box of the scene
 */
osg::BoundingBox calculateBoundingBox(osg::ref_ptr<osg::Node> scene);

/**
 * Calculates the length of the longest side of the given scene.
 * @param scene The scene we want to measure
 * @return The lenght of its longest side.
 */
double calcLongestSceneSide(osg::ref_ptr<osg::Node> scene);

void display_points_and_edges(const std::vector<std::vector<double> >* points=nullptr,
							  const std::vector<std::pair<std::vector<double>,std::vector<double> > >* edges=nullptr,
							  osg::ref_ptr<osg::Geode> g=nullptr);

osg::ref_ptr<osg::FloatArray> convertOsgVectorToArray(const osg::Vec3d& vector);

osg::ref_ptr<osg::Vec3dArray> convert2dVectorToOsgVec3dArray(const std::vector<std::vector<double> >& vec);

/**
 * Shows the center_node on the screen, with the camera rotating around it. Useful for getting an idea of the
 * structure of a plan, and for storing a nice-looking video of the plan.
 * @param center_node The node containing the 3D structure we want to visualize
 * @param orbit_distance The distance we want the camera to orbit the structure at
 * @param orbit_angle The tilt-angle the camera points with towards the structure while rotating.
 */
void run_rotating_camera(osg::ref_ptr<osg::Node> center_node, int orbit_distance=100.0, double orbit_angle=45.0);

}

#endif /* OSGHELPERS_H_ */
