/*
 * ImageCaptureTool.cpp
 *
 *  Created on: Apr 7, 2015
 *      Author: tiagotrocoli
 */

#include "ImageViewerCaptureTool.hpp"

#include <iostream>

//Trocoli's code. Kai changed two occurences of GL_FLOAT to GL_UNSIGNED_BYTE here, to grab the actual RGB color values.

namespace vizkit3d_normal_depth_map {

ImageViewerCaptureTool::ImageViewerCaptureTool(uint width, uint height) {

	std::cout << "Cam constructor " << width << ", " << height << std::endl;
    // initialize the hide viewer;
    initializeProperties(width, height);
}

ImageViewerCaptureTool::ImageViewerCaptureTool(double fovY, double fovX, uint height, double z_near, double z_far) {
    double aspectRatio = fovX / fovY;
    uint width = height * aspectRatio;
    initializeProperties(width, height);
    _viewer->getCamera()->setProjectionMatrixAsPerspective(fovY, aspectRatio, z_near, z_far);
}

void ImageViewerCaptureTool::initializeProperties(uint width, uint height) {

    // initialize the hide viewer;
    _viewer = new osgViewer::Viewer;
    osg::Camera *camera = this->_viewer->getCamera();
    //The following two lines enable us to clip away parts that are beyond the maximum depth we can observe.
    camera->setCullingMode(osg::CullSettings::VIEW_FRUSTUM_CULLING |osg::CullSettings::SMALL_FEATURE_CULLING);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = width;
    traits->height = height;
    traits->pbuffer = true;
    traits->readDISPLAY();
    traits->alpha = false; ///< I don't need alpha in my sensor simulator. Just want the color value.
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    camera->setGraphicsContext(gc);
    camera->setDrawBuffer(GL_FRONT);
    camera->setViewport(new osg::Viewport(0, 0, width, height));
    // initialize the class to get the image in float data resolution
    _capture = new WindowCaptureScreen(gc);
    _viewer->getCamera()->setFinalDrawCallback(_capture);
    //Setting the background in images to be white.
    _viewer->getCamera()->setClearColor(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    //osgViewer::ViewerBase::ThreadingModel threadingModel=osgViewer::ViewerBase::CullDrawThreadPerContext;//osgViewer::CompositeViewer::SingleThreaded;
    //_viewer->setThreadingModel(threadingModel);

}

osg::ref_ptr<osg::Image> ImageViewerCaptureTool::grabImage(const osg::ref_ptr<osg::Node> node) {
    _viewer->setSceneData(node);

    //std::clock_t    start = std::clock();
    _viewer->frame();
    //std::cout << "Time inside viewer frame: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return _capture->captureImage();
}

void ImageViewerCaptureTool::setCameraPosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up) {
    _viewer->getCamera()->setViewMatrixAsLookAt(eye, center, up);
}

void ImageViewerCaptureTool::getCameraPosition(osg::Vec3d& eye, osg::Vec3d& center, osg::Vec3d& up) {
    _viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
}

void ImageViewerCaptureTool::setBackgroundColor(osg::Vec4d color) {
    _viewer->getCamera()->setClearColor(color);
}

////////////////////////////////
////WindowCaptureScreen METHODS
////////////////////////////////

WindowCaptureScreen::WindowCaptureScreen(osg::ref_ptr<osg::GraphicsContext> gc) {

    _mutex = new OpenThreads::Mutex();
    _condition = new OpenThreads::Condition();
    _image = new osg::Image();

    // checks the GraficContext from the camera viewer
    if (gc->getTraits()) {


        GLenum pixelFormat;
        if (gc->getTraits()->alpha)
            pixelFormat = GL_RGBA;
        else
            pixelFormat = GL_RGB;


        int width = gc->getTraits()->width;
        int height = gc->getTraits()->height;

        // allocates the image memory space
        _image->allocateImage(width, height, 1, pixelFormat, GL_UNSIGNED_BYTE);
    }
}

WindowCaptureScreen::~WindowCaptureScreen() {
    delete (_condition);
    delete (_mutex);
}

osg::ref_ptr<osg::Image> WindowCaptureScreen::captureImage() {

    //wait to finish the capture image in call back
	//std::clock_t    start = std::clock();
    _condition->wait(_mutex);
    //std::cout << "Time spent waiting for mutex: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return _image;
}

void WindowCaptureScreen::operator ()(osg::RenderInfo& renderInfo) const {
    osg::ref_ptr<osg::GraphicsContext> gc = renderInfo.getState()->getGraphicsContext();
    if (gc->getTraits()) {
        _mutex->lock();
	    //std::clock_t    start = std::clock();
        _image->readPixels(0, 0, _image->s(), _image->t(), _image->getPixelFormat(), GL_UNSIGNED_BYTE);

	    //std::cout << "Time to image_readpix: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
        //grants the access to image
        _condition->signal();
        _mutex->unlock();
    }
}

}
