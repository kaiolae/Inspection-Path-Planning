/*
 * SceneKeeper.h
 *
 *  Created on: Apr 20, 2015
 *      Author: kaiolae
 *
 *      Some utility functions for managing scene graphs.
 */

#ifndef SCENEKEEPER_H_
#define SCENEKEEPER_H_

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <iostream>
#include <map>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ref_ptr>
#include <osg/TriangleFunctor>
#include <osg/Vec3d>
#include <osgDB/ReadFile>
#include <osgUtil/IntersectionVisitor>
#include <set>
#include <string>
#include <vector>

#include "TriangleData.h"

namespace utility_functions {

	/**
     * This class lets you load a 3D model, and then perform various useful operations on its scene graph.
     */
	class SceneKeeper {

	private:
		osg::ref_ptr<osg::Node> scene;
		osg::TriangleFunctor<TriangleData> triangleStore; ///<Structure holding info on all the triangles in the scene.
		double collisionPenalty; ///< The penalty given to any plan edge that collides with this scene.
		osg::Vec3d sceneCenter;
		osg::BoundingBox boudingBox;


		///Stores all triangles in the scene "drawable", into our TriangleFunctor triangleStore.
		void storeTriangles(osg::Geometry &drawable);

		/**
         * Method loading a scene from a 3D-model file into an osg-node. Once loaded, this SceneKeeper object lets us do many useful operations on that scene.
         * @param fileName The file path of the 3D model we want to load
         * @return A pointer to the root of the scenegraph holding our scene
         */
		osg::ref_ptr<osg::Node> LoadScene(const std::string &fileName);

		std::string loaded_scene_name = "";

	public:
		/**
         * Counts the number of primitives (mesh triangles) in the scene, and stores the result into our triangleStore. Information about each triangle is also store there,
         * such as its center and area.
         * This method should be called once for each run, before testing plans. Counting triangles is a prerequisite to calculating coverage scores.
         */
		void countTriangles();

		//Need to explicitly declare the default constructor when deleting the copy-constructor.
		SceneKeeper(const std::string &fileName);

		// = default;
		//Here, I'm disallowing copy-constructors for this object. Since this is a large object, we want to avoid copies, and rather use pointers.
		SceneKeeper(const SceneKeeper &) = delete;

		SceneKeeper &operator=(const SceneKeeper &) = delete;


		/**
         * Colors each triangle in the current TriangleStore differently, and returns the resulting scenegraph.
         * countTriangles is a prerequisite to running this method.
         * Note that the scene we received as input (member variable scene) is conserved. The coloring only affects the TriangleStore.
         * For that reason, the caller needs to maintain his own reference to the colored scene, if he wishes to use it.
         * @return The scenegraph after coloring each triangle differently
         */
		osg::ref_ptr<osg::Geode> colorEachTriangleDifferently();

		const size_t getTriangleCount() const {
			return triangleStore.getTriangleCount();
		}

		/**
         * Uses the mapping from color to triangle ID to calculate the coverage given covered colors.
         * Calculates the percentage of the current structure a given set of triangles cover, scaled to the interval 0-1, where 0 means all surfaces are covered, and 1 means no coverage.
         * @param coveredColors The colors that were observed during execution of a plan
         * @param g optional - if given, we plot the colored triangles
         * @return Coverage score
         */
		double calculateCoverage(const boost::dynamic_bitset<> coveredColors, osg::ref_ptr<osg::Geode> g = nullptr,
								 bool printerFriendly = true);


		osg::TriangleFunctor<TriangleData> &getTriangleStore() {
			return triangleStore;
		}


		osg::ref_ptr<osg::Node> getScene() const {
			return scene;
		}

		double getCollisionPenalty() const {
			return collisionPenalty;
		}

		const osg::Vec3d &getSceneCenter() const {
			return sceneCenter;
		}

		const osg::BoundingBox &getBoundingBox() const {
			return boudingBox;
		}

		const double getBoundingBoxVolume() const {
			return (boudingBox.xMax() - boudingBox.xMin()) * ((boudingBox.yMax() - boudingBox.yMin())) *
				   (boudingBox.zMax() - boudingBox.zMin());
		}

		double getSceneDiagonal() const {
			return (boudingBox._max - boudingBox._min).length();
		}

		const std::string &getLoaded_scene_name() const {
			return loaded_scene_name;
		}


	};

}/* namespace utility_functions */

#endif /* SCENEKEEPER_H_ */
