/*
 * GeodeFinder.h
 *
 *  Created on: Apr 14, 2015
 *      Author: kaiolae
 *
 *  Helper class to get Geodes from scenes - which is necessarry to count all geometric primitives of the scene.
 */

#ifndef GEODEFINDER_H_
#define GEODEFINDER_H_

#include <osg/Geode>
#include <osg/NodeVisitor>
#include <vector>


/**
 * A class that helps us find all the geodes in a scene.
 */
class GeodeFinder : public osg::NodeVisitor {
   public:

   // Constructor - sets the traversal mode to TRAVERSE_ALL_CHILDREN
   // and Visitor type to NODE_VISITOR
   GeodeFinder();

   // The 'apply' method for 'node' type instances.
   // See if a className() call to searchNode returns "Geode."
   // If so, add this node to our list.
   void apply(osg::Node &searchNode);

   // Return a pointer to the first node in the list
   // with a matching name
   osg::Geode* getFirst();

   // return a the list of nodes we found
   std::vector<osg::Geode*> getNodeList();

   private:
   // List of nodes with names that match the searchForName string
   std::vector<osg::Geode*> foundGeodes;
};

#endif /* GEODEFINDER_H_ */
