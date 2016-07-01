/*
 * CameraEstimator.h
 *
 *  Created on: Jun 16, 2015
 *      Author: kaiolae
 *
 *  Estimates the parts of the scene we can observe by sliding the camera across the edges of a plan.
 */

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <osg/Array>
#include <osg/Group>
#include <osg/Image>
#include <osg/Matrixd>
#include <osg/ref_ptr>
#include <osg/Vec3d>
#include <string>
#include <vector>

namespace vizkit3d_normal_depth_map {
class ImageViewerCaptureTool;
} /* namespace vizkit3d_normal_depth_map */

#ifndef CAMERAESTIMATOR_H_
#define CAMERAESTIMATOR_H_

namespace utility_functions {

/**
 * Stores a view of a drawable object to file as a picture. Essentially takes a snapshot of something we want to draw, and stores it to file.
 * @param scene The drawable object we want to take a picture of
 * @param viewMatrix The view we want to take the picture from
 * @param fileName The file path we wish to store the result to. If that file already exits, the filename is automatically
 * incremented (by incrementing any number at the end of the filename), until a non-existing file is found.
 */
void storeViewToFile(const osg::ref_ptr<osg::Node> scene, const osg::Matrixd& viewMatrix, std::string& fileName);

class CameraEstimator {

private:

	///The distance between each simulated snapshot which is used to estimate coverage. Smaller distances give a slower algorithm (due to more rendered snapshots), but more accurate
	///estimates of coverage.
	double distanceBetweenCameraSnapshots;
	double fov_x;				///<The x-field of view of the camera in degrees.
	double fov_y;				///<The y-field of view of the camera in degrees.
	double z_near;				///<The distance from the camera to the nearest point we can see in the z-direction.
	double z_far;				///<The distance from the camera to the furthest point we can see in the z-direction.
	unsigned int image_height;	///<The height of the image in pixels.

	///We can simulate the robot with either camera alone, or both. No point simulating without cameras here.
	bool usingFrontCamera;
	bool usingBelowCamera;

	///An object we use to render and store images.
	vizkit3d_normal_depth_map::ImageViewerCaptureTool* capture;

	/**
	 * Calculates all the positions along the current edge where we should render images, for use in coverage estimates.
	 * @param startLocation The start point of the edge
	 * @param endLocation The end point of the edge
	 * @param[out] samplingPositions The viewpoints sampled along the edge, returned as reference
	 */
	void getSamplingPositions(const osg::Vec3d& startLocation, const osg::Vec3d& endLocation, osg::Vec3Array& samplingPositions, double sampling_interval) const;// osg::ref_ptr<osg::Vec3Array> samplingPositions);

	/**
	 * Finds and stores all unique colors present in a given image
	 * @param image The input image
	 * @param[out] ObservedColors A bitset representing each possible RGB color, with a 1 if that color was present in the image, and 0 otherwise. Returned by reference.
	 */
	void getAllColorsInFrame(const osg::Image& image, boost::dynamic_bitset<>& ObservedColors) const;

public:



	//Here, I'm disallowing copy-constructors for this object. Since this is a large object, we want to avoid copies, and rather use pointers.
	CameraEstimator(const CameraEstimator&) = delete;
	CameraEstimator& operator=(const CameraEstimator&) = delete;

	/**
	 * Constructs a camera estimator with the given specifications.
	 * @param cameraSpecs Specifications for the camera. Inputs are interpreted as:
	 * 	cameraSpecs[0] = distanceBetweenCameraSnapshots;
	 *	cameraSpecs[1] = fov_x;
	 *  cameraSpecs[2] = fov_y;
	 *	cameraSpecs[3] = image_height;
	 *	cameraSpecs[4] = z_near;
	 *	cameraSpecs[5] = z_far;
	 *	cameraSpecs[6] = using_front_camera;
	 *	cameraSpecs[7] = using_below_camera;
	 */
	CameraEstimator(const std::vector<double>& cameraSpecs);

	/**
	 * Default constructor. When not giving camera specifications, they are defaulted to the values given in Constants.h
	 */
	CameraEstimator();

	virtual ~CameraEstimator();

	/**
	 * Finds all colors seen by traversing the scene across the given edge, updating a bitset representing all the colors.
	 * @param currentLocation The start point of the traversal
	 * @param nextLocation The end point of the traversal
	 * @param robotHeading The direction the robot is pointing while traversing. This is the same direction as the front-facing camera will point, and also the same as the
	 * "up-direction" of the downwards facing camera.
	 * @param inspectionTarget The object we are inspecting. NB: It is assumed that each triangle in this object is colored in a unique color.
	 * @param[out] observedColors A bitset with a bit for each possible RGB color. All observed colors get their bit set to 1, the others stay at 0. Returned by reference.
	 * @param sampling_interval (optional) The distance between snapshots along the edge, used to measure what the edge covers. Lower values gives
	 * more realistic estimates, but also lead the algorithm to taking more time. If no value is given, the default (distancebetweenCameraSnasots)
	 * for this object is used.
	 * @param drawCameraImageTo Optional parameter with a OSG-group that we can draw the camerea's image to. Useful for testing, to "see what the camera sees".
	 */
	void GetColorsDuringTraversal(const osg::Vec3d& currentLocation, const osg::Vec3d& nextLocation, const osg::Vec3d& robotHeading,
			const osg::ref_ptr<osg::Node> inspectionTarget, boost::dynamic_bitset<>& observedColors, double sampling_interval,
								  osg::ref_ptr<osg::Group> drawCameraImageTo = nullptr) const;

	//Overriding method for setting sampling_interval default when not explicitly specified.
	void GetColorsDuringTraversal(const osg::Vec3d& currentLocation, const osg::Vec3d& nextLocation, const osg::Vec3d& robotHeading,
								  const osg::ref_ptr<osg::Node> inspectionTarget, boost::dynamic_bitset<>& observedColors,
								  osg::ref_ptr<osg::Group> drawCameraImageTo = nullptr) const{
		GetColorsDuringTraversal(currentLocation, nextLocation, robotHeading, inspectionTarget, observedColors,
								 distanceBetweenCameraSnapshots, drawCameraImageTo);
	}


	void setDistanceBetweenCameraSnapshots(
			double distanceBetweenCameraSnapshots) {
		this->distanceBetweenCameraSnapshots = distanceBetweenCameraSnapshots;
	}

	double getDistanceBetweenCameraSnapshots() const {
		return distanceBetweenCameraSnapshots;
	}
};

} /* namespace utility_functions */

#endif /* CAMERAESTIMATOR_H_ */
