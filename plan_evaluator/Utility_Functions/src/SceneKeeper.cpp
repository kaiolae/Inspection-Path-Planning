/*
 * SceneKeeper.cpp
 *
 *  Created on: Apr 20, 2015
 *      Author: kaiolae
 */

#include "SceneKeeper.h"

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/Optimizer>

#include "Constants.h"
#include "GeodeFinder.h"
#include "OsgHelpers.h"

namespace utility_functions {


double SceneKeeper::calculateCoverage(const boost::dynamic_bitset<> coveredColors, osg::ref_ptr<osg::Geode> g, bool printerFriendly){
	return triangleStore.calculateAndColorCoverage(coveredColors, g, printerFriendly);
}


// decompose Drawable primtives into triangles, print out these triangles and computed normals.
void SceneKeeper::storeTriangles(osg::Geometry& drawable)
{
	//Simple, thanks to OSG's visitor pattern.
    drawable.accept(triangleStore);
	triangleStore.setTriangle_scene_name(loaded_scene_name);
}


void SceneKeeper::countTriangles(){
	//First, finding the geode(s) from the scene.
	GeodeFinder geodeFinder;
	scene->accept(geodeFinder);
	std::vector<osg::Geode*> allGeodes = geodeFinder.getNodeList();

	for(std::vector<osg::Geode*>::iterator it = allGeodes.begin(); it!=allGeodes.end();it++){
	   osg::Geode* currentGeode = *it;
	   for(unsigned int drawableNum = 0; drawableNum < currentGeode->getNumDrawables(); drawableNum++){


		   //This turns the scene into a set of triangles, rather than a set of "triangle strips".
			osgUtil::Optimizer opt = osgUtil::Optimizer();
			opt.optimize(currentGeode,osgUtil::Optimizer::INDEX_MESH||osgUtil::Optimizer::VERTEX_PRETRANSFORM||osgUtil::Optimizer::VERTEX_POSTTRANSFORM);
		   osg::ref_ptr<osg::Geometry> geometry = (osg::Geometry *) currentGeode->getDrawable(drawableNum);

		   //Storing all triangles
		   this->storeTriangles(*geometry);
		   triangleStore.calculateTotalArea();

	   }
	}
}

osg::ref_ptr<osg::Geode> SceneKeeper::colorEachTriangleDifferently(){
	if (triangleStore.getTriangleCount() == 0){
		countTriangles();
	}
	return triangleStore.colorEachTriangleDifferently();
}

SceneKeeper::SceneKeeper(const std::string& fileName){
	scene = LoadScene(fileName);
	collisionPenalty = COLLISION_PENALTY_MODIFIER*calcLongestSceneSide(scene);

	sceneCenter = scene->getBound()._center;
	boudingBox = calculateBoundingBox(scene);
}

osg::ref_ptr<osg::Node> SceneKeeper::LoadScene(const std::string& fileName){
	//Use KD-trees to build the model. The KD tree is automatically used by intersectors.
	//Using the KD-tree to ray trace gives tremendous speedups - we observed between 10 times and 100 times, depending on the exact model.
	osgDB::Registry::instance()->setBuildKdTreesHint(osgDB::ReaderWriter::Options::BUILD_KDTREES);
	osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(fileName);

	if (!scene)
	{
		throw std::invalid_argument( "The following file does not specify a valid 3D model for OpenSceneGraph: " + fileName );
	}

	loaded_scene_name = fileName;
	return scene;
}

} /* namespace utility_functions */


