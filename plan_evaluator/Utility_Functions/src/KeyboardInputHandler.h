/*
 * KeyboardInputHandler.h
 *
 *  Created on: Jul 22, 2015
 *      Author: kaiolae
 */

#ifndef KEYBOARDINPUTHANDLER_H_
#define KEYBOARDINPUTHANDLER_H_

#include <iostream>
#include <osg/Matrixd>
#include <osgGA/GUIEventHandler>

namespace utility_functions {

///Used to store viewMatrices the user selects, synchronizing between the GUI event and the viewer-loop.
class viewMatrixStorage
{

	   osg::Matrixd mostRecentViewMatrix;
	   bool matrixWasSet;
public:
	   viewMatrixStorage(){
		   matrixWasSet = false;
	   }

	   void updateViewMatrix(osg::Matrixd mat){
		   mostRecentViewMatrix = mat;
		   std::cout << "vms update" << std::endl;
		   matrixWasSet=true;
	   }

	const osg::Matrixd& getMostRecentViewMatrix() const {
		return mostRecentViewMatrix;
	}
};



class KeyboardInputHandler: public osgGA::GUIEventHandler {
protected:
	viewMatrixStorage* viewStorage;

public:
    virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&);
    virtual void accept(osgGA::GUIEventHandlerVisitor& v)   { v.visit(*this); };

	KeyboardInputHandler(viewMatrixStorage* vms){
		viewStorage = vms;
	}
};

} /* namespace evolutionary_inspection_plan_evaluation */

#endif /* KEYBOARDINPUTHANDLER_H_ */
