/*
 * KeyboardInputHandler.cpp
 *
 *  Created on: Jul 22, 2015
 *      Author: kaiolae
 */

#include "KeyboardInputHandler.h"

namespace utility_functions {

//If we return 'true' the event is considered handled and will not be passed on to other handlers.
//If we return 'false' other handlers will have the opportunity to respond to that event.
bool KeyboardInputHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa)
     {
       switch(ea.getEventType())
       {
       case(osgGA::GUIEventAdapter::KEYDOWN):
          {
             switch(ea.getKey())
             {
             case 'w':
             {
                std::cout << " w key pressed" << std::endl;

        		osg::Vec3 eye, centre, up;
        		osg::View* viewer = aa.asView();
        		osg::Matrixd viewMatrix = viewer->getCamera()->getViewMatrix();
        		viewStorage->updateViewMatrix(viewMatrix);

                return false;
             }
             default:
                return false;
             }
          }
       default:
          return false;
       }
    }

} /* namespace evolutionary_inspection_plan_evaluation */
