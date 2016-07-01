/*
 * CameraEstimator.cpp
 *
 *  Created on: Jun 16, 2015
 *      Author: kaiolae
 */

#include "CameraEstimator.h"

#include <boost/filesystem.hpp>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LightModel>
#include <osg/Texture2D>
#include <osgDB/WriteFile>

#include "Constants.h"
#include "HelperMethods.h"
#include "ImageViewerCaptureTool.hpp"
#include "OsgHelpers.h"


namespace utility_functions {

// This function builds a textured quad. Used for testing, as it can be useful to draw the camera's image on.
osg::Node* build_quad(osg::Texture2D *tex)
{
    osg::Geometry *geo = new osg::Geometry;
    osg::Vec3Array *vx = new osg::Vec3Array;
    vx->push_back(osg::Vec3(-10, 0, -10));
    vx->push_back(osg::Vec3(10, 0, -10));
    vx->push_back(osg::Vec3(10, 0, 10));
    vx->push_back(osg::Vec3(-10, 0, 10));
    geo->setVertexArray(vx);
    osg::Vec3Array *nx = new osg::Vec3Array;
    nx->push_back(osg::Vec3(0, -1, 0));
    geo->setNormalArray(nx);
    geo->setNormalBinding(osg::Geometry::BIND_OVERALL);
    osg::Vec2Array *tx = new osg::Vec2Array;
    tx->push_back(osg::Vec2(0, 0));
    tx->push_back(osg::Vec2(1, 0));
    tx->push_back(osg::Vec2(1, 1));
    tx->push_back(osg::Vec2(0, 1));
    geo->setTexCoordArray(0, tx);
    geo->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));
    geo->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex);

    osg::Geode *geode = new osg::Geode;
    geode->addDrawable(geo);
    return geode;
}

CameraEstimator::CameraEstimator(const std::vector<double>& cameraSpecs){
	distanceBetweenCameraSnapshots = cameraSpecs[0];
	fov_x = cameraSpecs[1];
	fov_y = cameraSpecs[2];
	image_height = (unsigned int) cameraSpecs[3];
	z_near = cameraSpecs[4];
	z_far = cameraSpecs[5];
	usingFrontCamera = cameraSpecs[6];
	usingBelowCamera = cameraSpecs[7];
	capture = new vizkit3d_normal_depth_map::ImageViewerCaptureTool(fov_x,fov_y,image_height,z_near,z_far);
}

CameraEstimator::CameraEstimator(){
	distanceBetweenCameraSnapshots = DEFAULT_SAMPLING_INTERVAL_CAM;
	fov_x = FOV_HORIZONTAL;
	fov_y = FOV_VERTICAL;
	image_height = DEFAULT_CAM_HEIGHT;
	z_near = CAMERA_NEAR_PLANE_DIST;
	z_far = CAMERA_FAR_PLANE_DIST;
	usingFrontCamera = FORWARD_CAMERA_ACTIVE;
	usingBelowCamera = DOWNWARD_CAMERA_ACTIVE;
	capture = new vizkit3d_normal_depth_map::ImageViewerCaptureTool(fov_x,fov_y,image_height,z_near,z_far);
}




CameraEstimator::~CameraEstimator() {
}

void CameraEstimator::getSamplingPositions(const osg::Vec3d& startLocation, const osg::Vec3d& endLocation, osg::Vec3Array& samplingPositions, double sampling_interval) const{//osg::ref_ptr<osg::Vec3Array> samplingPositions){

	//Using default if none is given.
	if(sampling_interval<=0){
		sampling_interval = distanceBetweenCameraSnapshots;
	}
	osg::Vec3d movementVector = endLocation-startLocation;
	samplingPositions.push_back(startLocation);
	osg::Vec3d nextLocation = startLocation + (movementVector*sampling_interval)/double(movementVector.length());
	while((nextLocation-startLocation).length() < movementVector.length()){//Until we reach the end of the movement
		samplingPositions.push_back(nextLocation);
		nextLocation += (movementVector*sampling_interval)/movementVector.length();
	}
	samplingPositions.push_back(endLocation);
}

void CameraEstimator::getAllColorsInFrame(const osg::Image& image, boost::dynamic_bitset<>& ObservedColors) const{
    int column_start = 0;
    int column_end = image.s();
    int row_start = 0;
    int row_end = image.t();
    if (image.getPixelFormat()==GL_RGBA){
    	std::cerr << "WARNING: Picture format RGBA, expected RGB" << std::endl;
    }
    if(image.getDataType()==GL_FLOAT){
    	std::cerr << "WARNING: Picture data of type GL_FLOAT, expected GL_UNSIGNED_INT" << std::endl;
    }
    //std::clock_t    start = std::clock();
    std::map<osg::Vec3, int> colorObservations;
    for(int r=row_start; r<row_end; ++r)
    {
    	const unsigned char* data = image.data(column_start, r);
        for(int c=column_start; c<column_end; ++c)
        {
        	const unsigned int r = *data;	++data;
        	const unsigned int g = *data; ++data;
        	const unsigned int b = *data; ++data;

        	if(ObservedColors.size()<=(r*256*256)+(g*256)+b){
        		//We observed a color we were did not color our model with. Can for instance happen when we see OSG's background color. Just skipping that color.
        		continue;
        	}
			ObservedColors.set((r*256*256)+(g*256)+b); //Marks the color as observed.
        }
    }
    //std::cout << "Time to read pixels: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
}

void CameraEstimator::GetColorsDuringTraversal(const osg::Vec3d& currentLocation, const osg::Vec3d& nextLocation,
	const osg::Vec3d& robotHeading, const osg::ref_ptr<osg::Node> inspectionTarget, boost::dynamic_bitset<>& observedColors,
											   double sampling_interval,
											   osg::ref_ptr<osg::Group> drawCameraImageTo /*=nullptr*/)
											    const{
	osg::ref_ptr<osg::Vec3Array> samplingPositions = new osg::Vec3Array();
	if(currentLocation==nextLocation){
		//If we're only given a single waypoint, we just sample colors once, right at this waypoint.
		samplingPositions->push_back(currentLocation);
	}
	else{
		getSamplingPositions(currentLocation, nextLocation, *samplingPositions, sampling_interval);
	}
	osg::ref_ptr<osg::Texture2D> tex; //Texture to draw the camera's image to, for testing/debugging.
	if(drawCameraImageTo!=nullptr){
		tex = new osg::Texture2D;

	}
	for(osg::Vec3Array::iterator it = samplingPositions->begin(); it<samplingPositions->end(); it++){
		osg::Vec3 point = *it;
		if(usingFrontCamera){
			capture->setCameraPosition(point, point+robotHeading, UP_VECTOR);
			osg::ref_ptr<osg::Image> osgImage = capture->grabImage(inspectionTarget);
			getAllColorsInFrame(*osgImage, observedColors);
			if(drawCameraImageTo!=nullptr){
				tex->setTextureSize(osgImage->s(),osgImage->t());
				tex->setInternalFormat(GL_RGB); //requests 8 bits per color component, which corresponds to 256 possible values each for R, G and B.
				tex->setImage(0, osgImage);
			}

		}
		if(usingBelowCamera){
			//A camera looking down has lookAt straight down, and "up-vector" in front of the robot.
			capture->setCameraPosition(point,point-UP_VECTOR, robotHeading);
			osg::ref_ptr<osg::Image> osgImage = capture->grabImage(inspectionTarget);
			getAllColorsInFrame(*osgImage, observedColors);
		}
		//std::clock_t    start = std::clock();
		//std::cout << "Time to render image: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
		//std::clock_t    start2 = std::clock();
		//std::cout << "Time to get image colors: " << (std::clock() - start2) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;


	}
	if(drawCameraImageTo!=nullptr){
		drawCameraImageTo->addChild(build_quad(tex.get()));
	}
}



void storeViewToFile(const osg::ref_ptr<osg::Node> scene, const osg::Matrixd& viewMatrix, std::string& fileName){

	//Making sure all surfaces are lit from both sides, in case of wrongly flipped normals.
	osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
	lightModel->setTwoSided(true);
	scene->getOrCreateStateSet()->setAttributeAndModes(lightModel.get());

	vizkit3d_normal_depth_map::ImageViewerCaptureTool* screenGrabber = new vizkit3d_normal_depth_map::ImageViewerCaptureTool(IMG_WIDTH,IMG_HEIGHT);
	std::cout << "grapper set up" << std::endl;
	screenGrabber->setViewMatrix(viewMatrix);
	std::cout << "matrix set up" << std::endl;
	osg::ref_ptr<osg::Image> capturedImage = screenGrabber->grabImage(scene);
	std::cout << "image grabbed" << std::endl;


	osgDB::writeImageFile(*capturedImage,fileName);
	delete screenGrabber;
}

} /* namespace utility_functions */
