/*
 * TriangleData.h
 *
 *  Created on: May 26, 2015
 *      Author: kaiolae
 */

#ifndef TRIANGLEDATA_H_
#define TRIANGLEDATA_H_

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <stddef.h>
#include <osg/Array>
#include <osg/Geode>
#include <osg/ref_ptr>
#include <osg/Vec3d>
#include <osg/Vec4>
#include <set>
#include <vector>

namespace utility_functions {


/**
 * Helper class to find and store all triangles in a scene. Follows the "visitor" pattern of OSG, which requires us to implement the
 * function "operator", which will be called each time a triangle is visited.
 */
class TriangleData
{
private:

	double totalArea = 0; ///<Total area of all triangles - necessary to get the percentage of the area we currently cover.
	std::vector<std::vector<osg::Vec3d> > triangles;	///<Vector containing each triangle in the scene.
	std::vector<double> triangleSizes; 	///<Stores the size of each triangle in the scene, mapped from that triangle's ID.
	std::vector<osg::Vec3d> triangleCenters; 	///<The center point of each triangle.

	/**
	 * Colors the given triangle with the given color, and inserting into the given geode for display.
	 * Useful during plotting, to show which parts are covered and not.
	 * @param triangleIndex The index of the triangle we want to color
	 * @param g The geode we want to draw to
	 * @param color The color to give to the triangle
	 */
	void colorCoveredTriangle(int triangleIndex, osg::Geode& g, const osg::Vec4& color) const;
	/**
	 * Generates numColors different unique RGB colors, from 0,0,0 - 0,0,1 - ... 255,255,255
	 * @param numColors The number of colors we want to generate
	 * @return a vector containing each unique color
	 */
	osg::ref_ptr<osg::Vec3Array> generateDifferentRandomColors(int numColors) const;

	std::string triangle_scene_name; //The name of the 3d model file these triangles came from

public:

	//Need to explicitly declare the default constructor when deleting the copy-constructor.
	TriangleData() =default;
	//Here, I'm disallowing copy-constructors for this object. Since this is a large object, we want to avoid copies, and rather use pointers.
	TriangleData(const TriangleData&) = delete;
	TriangleData& operator=(const TriangleData&) = delete;

	/**triangles
	 * OSG visitor pattern ensures operator() is called once for every triangle.
	 * In this method, each triangle is stored, along with information about its center and size.
	 * @param v1 Corner 1 of the triangle
	 * @param v2 Corner 2 of the triangle
	 * @param v3 Corner 3 of the triangle
	 * @param Unused bool - needed to comply with visitor-pattern.
	 */
	void operator()(const osg::Vec3d v1, const osg::Vec3d v2, const osg::Vec3d v3, bool treatVertexDataAsTemporary);

	/**
	 * Calculates the summed area of all the triangles in the scene, and stores them in the "totalArea" variable.
	 */
	void calculateTotalArea();

	/**
	 * Assigns a different color to each triangle in the current store, and returns a colored scene where each triangle has a different color
	 * @param coloredScene A scenegraph with the loaded model, where each triangle has a different color.
	 */
	osg::ref_ptr<osg::Geode> colorEachTriangleDifferently() const;

	/**
	 * Calculates the percentage of the 3D-model the covered points represent (area-wise).
	 * The result is a number between 0 and 1, 0 meaning full coverage and 1 meaning no coverage.
	 * This return value frames optimization as minimization, which is beneficial, since we want to minimize energy usage,
	 * and not dealing with different "directions" of optimization simplifies things.
	 * @param coveredColors All the colors that were seen during the inspection mission
	 * @param g Optional- for plotting the coverage
	 * @return Coverage score between 0 and 1, 0 meaning full coverage and 1 meaning no coverage.
	 */
	double calculateAndColorCoverage(const boost::dynamic_bitset<> &coveredColors, osg::ref_ptr<osg::Geode> g = nullptr,
									 bool printerFriendly = false) const;



	const std::vector<osg::Vec3d>& getTriangleCenters() const {
		return triangleCenters;
	}

	const std::vector<std::vector<osg::Vec3d> >& getTriangles() const {
		return triangles;
	}

	const osg::Vec3d& getTriangleCenter(size_t triangleId) const{
		return triangleCenters[triangleId];
	}

	const size_t getTriangleCount() const{
		return triangles.size();
	}


	void setTriangle_scene_name(const std::string &triangle_scene_name) {
		TriangleData::triangle_scene_name = triangle_scene_name;
	}

};

} /* namespace utility_functions */

#endif /* TRIANGLEDATA_H_ */
