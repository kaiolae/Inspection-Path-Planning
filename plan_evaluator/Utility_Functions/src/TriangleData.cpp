/*
 * TriangleData.cpp
 *
 *  Created on: May 26, 2015
 *      Author: kaiolae
 */

#include "TriangleData.h"

#include <iostream>
#include <osg/Geometry>
#include <osgUtil/SmoothingVisitor>

#include "OsgHelpers.h"

namespace utility_functions {

//Assigning color values needs to be done from 0 to 1. Therefore, I need to convert my RGB values to floats.
float convertRGBValuesToFloats(int rgbValue){
	return float(rgbValue)/255.0;
}

osg::ref_ptr<osg::Vec3Array> TriangleData::generateDifferentRandomColors(int numColors) const{
	osg::ref_ptr<osg::Vec3Array> colors = new osg::Vec3Array();
	//TODO: Make sure I don't start from 0 if I assign colors again for some other object. So far not a problem I'm facing.
	for(unsigned int red = 0; red< 256; red++){
		for(unsigned int green = 0; green < 256; green++){
			for(unsigned int blue = 0; blue < 256; blue++){
				colors->push_back(osg::Vec3(red,green,blue));
				if(colors->size()== (unsigned)numColors){
					return colors;
				}
			}
		}
	}
	throw std::logic_error("The input 3D structure has too many primitives. We support a maximum of (256^3) primitives - in other words,"
								   " more than 16 million. If you have more than this, please either divide your problem, or use a different method.");

}

void TriangleData::operator()(const osg::Vec3d v1, const osg::Vec3d v2, const osg::Vec3d v3, bool treatVertexDataAsTemporary)
{
	//osg::Vec3dArray tri;
	std::vector<osg::Vec3d> tri;
	tri.push_back(v1);
	tri.push_back(v2);
	tri.push_back(v3);

	triangles.push_back(tri);

	//Calculating triangle centroid
	osg::Vec3d triangleCentroid((v1.x()+v2.x()+v3.x())/3.0,(v1.y()+v2.y()+v3.y())/3.0,(v1.z()+v2.z()+v3.z())/3.0);
	triangleCenters.push_back(triangleCentroid);

	//Calculating triangle size using Heron's formula
	double side1Len = (v2-v1).length();
	double side2Len = (v3-v1).length();
	double side3Len = (v3-v2).length();
	double halfPerimeter = (side1Len+side2Len+side3Len)/2.0;
	double triangleArea = sqrt(halfPerimeter*(halfPerimeter-side1Len)*(halfPerimeter-side2Len)*(halfPerimeter-side3Len));
	triangleSizes.push_back(triangleArea);

}

void TriangleData::calculateTotalArea(){
	std::cout << "Stored in total " << triangles.size() << " triangles" << std::endl;
	double area = 0;
	for(std::vector<double>::iterator it = triangleSizes.begin(); it<triangleSizes.end();it++){
		area+=*it;
	}
	totalArea = area;
}

//void TriangleData::divideScene(osg::ref_ptr<osg::Geode> inspectionScene, osg::ref_ptr<osg::Geode> collisionScene, osg::Vec3d inspectionColor){
//
//}

osg::ref_ptr<osg::Geode> TriangleData::colorEachTriangleDifferently() const{

	osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array;
	osg::ref_ptr<osg::Vec3Array> colorArray = new osg::Vec3Array;
	osg::ref_ptr<osg::Geode> coloredScene = new osg::Geode();
	osg::Geometry* coloredGeom = new osg::Geometry();
	coloredScene->addDrawable(coloredGeom);
	coloredGeom->setVertexArray(vertexArray);
	coloredGeom->setColorArray(colorArray.get(), osg::Array::BIND_PER_VERTEX);

	osg::ref_ptr<osg::Vec3Array> colors = generateDifferentRandomColors(triangles.size());
	int counter = 0;
	int primitiveCounter = 0;

	for(std::vector<std::vector<osg::Vec3d> >::const_iterator  it = triangles.begin(); it!=triangles.end(); it++){
		osg::ref_ptr<osg::DrawElementsUInt> trianglePrimitive = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
		const std::vector<osg::Vec3d> currentTriangle = *it;
		for(unsigned int i = 0; i<currentTriangle.size();i++){
			vertexArray->push_back(currentTriangle.at(i));
			trianglePrimitive->push_back(primitiveCounter);
			osg::Vec3 intColor = colors->at(counter);
			osg::Vec3 floatColor(convertRGBValuesToFloats(intColor.x()),convertRGBValuesToFloats(intColor.y()),convertRGBValuesToFloats(intColor.z()));

			colorArray->push_back(floatColor);

			primitiveCounter++;
		}
		coloredGeom->addPrimitiveSet(trianglePrimitive);
		counter += 1;
	}

	//Turning off the light. This makes the colors we see independent of normal directions, which is important when checking which colors we observe.
	osg::StateSet* state = coloredScene->getOrCreateStateSet();
	state->setMode( GL_LIGHTING,osg::StateAttribute::OFF |osg::StateAttribute::PROTECTED );

	return coloredScene;
}


void TriangleData::colorCoveredTriangle(int triangleIndex, osg::Geode& g, const osg::Vec4& color) const{
	std::vector<osg::Vec3d> coveredTriangle = triangles[triangleIndex];
	double triangle_z_level = triangleCenters[triangleIndex].z();
	osg::Vec4 scaled_color;

	//"Hack" I use to see the structure of the inspection target better in plots.
	if(triangle_scene_name.find("manifold") != std::string::npos && triangle_z_level < 1.52){
		scaled_color = color*0.5;
	}
	else{
		scaled_color = color;
	}

	osg::ref_ptr<osg::Vec3dArray> coveredTriangleArray = new osg::Vec3dArray();
	for(unsigned int i = 0; i<coveredTriangle.size();i++){
		coveredTriangleArray->push_back(coveredTriangle.at(i));
	}
	osg::Geometry* polyGeom = new osg::Geometry();

	//Calculating normals, to make the scene get a more three-dimensional view
	osg::ref_ptr<osg::Vec3dArray> normalArray = new osg::Vec3dArray();
	osg::Vec3d triangleNormal = calculateTriangleNormal(coveredTriangle[0],coveredTriangle[1],coveredTriangle[2]);
	for(unsigned int i = 0; i<coveredTriangle.size();i++){
		normalArray->push_back(triangleNormal);
	}


	polyGeom->setVertexArray(coveredTriangleArray);
	osg::Vec4Array* colors = new osg::Vec4Array;
	colors->push_back(scaled_color);
	polyGeom->setColorArray(colors, osg::Array::BIND_OVERALL);
	polyGeom->setNormalArray(normalArray);
	polyGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);
	polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,0,3));
	g.addDrawable(polyGeom);
}


double TriangleData::calculateAndColorCoverage(const boost::dynamic_bitset<> &coveredColors,
											   osg::ref_ptr<osg::Geode> g/*=nullptr*/, bool printerFriendly /*=false*/) const{
	double coverage = 0;
	for(unsigned int colorId=0;colorId<coveredColors.size();colorId++){

		if(coveredColors[colorId]){
			coverage+=triangleSizes[colorId];
			if(g!=nullptr){ //If we get a geode, we draw the covered triangles in red.
				if(printerFriendly){
					colorCoveredTriangle(colorId, *g, osg::Vec4(0.33f, 0.33f, 0.33f, 1.0f));
				}
				else{
					colorCoveredTriangle(colorId, *g, osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
				}
			}
		}
		else{
			if(g!=nullptr&&colorId<triangleSizes.size()){ //Non-covered triangles we draw in green
				if(printerFriendly){
					colorCoveredTriangle(colorId, *g, osg::Vec4(0.75f, 0.75f, 0.75f, 1.0f));
				}
				else{
					colorCoveredTriangle(colorId, *g, osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f));
				}
			}
		}

	}

	//Trying to add some sensible normals to those triangles, to make the scene look a bit better.

	if(g!=nullptr){
		osgUtil::SmoothingVisitor sv;
		g->accept(sv); //Smoothing normals
	}

	//colorCoveredTriangle(colorId,g);

	return 1.0 - (coverage/totalArea);
}

} /* namespace utility_functions */


