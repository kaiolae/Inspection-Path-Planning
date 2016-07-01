/*
 * main.cpp
 *
 *  Created on: Mar 27, 2015
 *      Author: kaiolae
 */

#include <iostream>
#include <osg/Geode>
#include <osgUtil/Optimizer>

#include "../../Utility_Functions/src/Constants.h"
#include "../../Utility_Functions/src/HelperMethods.h"
#include "../../Utility_Functions/src/OsgHelpers.h"
#include "PlanCoverageEstimator.h"

using namespace evolutionary_inspection_plan_evaluation;
using namespace utility_functions;

//A main-method for testing the program. Note that normally, the methods in PlanCoverageEstimator are instead accessed from some plan optimizer.
int main(int argc, char **argv)
{
	//Testing link to subfolder.
	//CompleteCoverage cc;
	//cc.calculateCompleteCoverage();
    std::string mandLoopsFile1  = "/home/kai/workspace/bir/bir_workspace/Coverage_Path_Planning/Complete_Coverage/releaseWithDebugInfo/test.dat";
	//std::string mandLoopsFile1  = "/home/kai/PycharmProjects/pycharm_projects/deap_framework/complete_coverage_paths/four_pegs_loop//complete_coverage_manifold_detail1.obj.dat";
	//std::string mandLoopsFile2  = "/home/kai/PycharmProjects/pycharm_projects/deap_framework/complete_coverage_paths/four_pegs_loop//complete_coverage_manifold_detail2.obj.dat";
	//std::string mandLoopsFile3  = "/home/kai/PycharmProjects/pycharm_projects/deap_framework/complete_coverage_paths/four_pegs_loop//complete_coverage_manifold_detail3.obj.dat";
	//std::string mandLoopsFile4  = "/home/kai/PycharmProjects/pycharm_projects/deap_framework/complete_coverage_paths/four_pegs_loop//complete_coverage_manifold_detail4.obj.dat";
	//"/home/kaiolae/PycharmProjects/deap_framework/complete_coverage_paths/test_paths/complete_coverage_manifold_detail2.obj.dat";
    std::vector<std::string> *mandLoopsFiles = new std::vector<std::string>;
    //mandLoopsFiles->push_back(mandLoopsFile1);
    mandLoopsFiles->push_back(mandLoopsFile1);
    //mandLoopsFiles->push_back(mandLoopsFile2);
    //mandLoopsFiles->push_back(mandLoopsFile3);
    //mandLoopsFiles->push_back(mandLoopsFile4);
    //mandLoopsFiles->push_back(mandLoopsFile3);
    //mandLoopsFiles->push_back(mandLoopsFile4);

	osg::ref_ptr<osg::Group> root(new osg::Group);

	osg::ref_ptr<osg::Geode> geode(new osg::Geode());


	double frustSpecs[] = {1.0, 1.0, 1.0, 10.0, 10.0, 10.0};
	std::vector<double> frustumSpecs (frustSpecs, frustSpecs + sizeof(frustSpecs) / sizeof(double) );


	double camSpecs[] = {DEFAULT_SAMPLING_INTERVAL_CAM, FOV_HORIZONTAL, FOV_VERTICAL, DEFAULT_CAM_HEIGHT, CAMERA_NEAR_PLANE_DIST, CAMERA_FAR_PLANE_DIST, FORWARD_CAMERA_ACTIVE, DOWNWARD_CAMERA_ACTIVE};
	std::vector<double> cameraSpecs (camSpecs, camSpecs + sizeof(camSpecs) / sizeof(double) );
	std::cout << "Cam specs generated. size: " << cameraSpecs.size() << std::endl;

	osgUtil::Optimizer opt = osgUtil::Optimizer();
	//opt.optimize(scene);//,osgUtil::Optimizer::INDEX_MESH||osgUtil::Optimizer::VERTEX_PRETRANSFORM||osgUtil::Optimizer::VERTEX_POSTTRANSFORM);

	//root->addChild(scene); //Uncomment to show the object we are scanning.
	root->addChild(geode);
    //osg::Timer_t startTick = osg::Timer::instance()->tick();
    double sensOrg[] = {-5,-5,0};
    std::vector<double> sensorOrigin (sensOrg, sensOrg + sizeof(sensOrg) / sizeof(double) );
    std::vector<std::vector<double> > plan;

//    double firstStep[] = {0,-6,M_PI,0.0};
    double firstStep[] = {89,0};//, 4.79609};
    std::vector<double> firstPlanStep (firstStep, firstStep + sizeof(firstStep) / sizeof(double) );

    //plan.push_back(firstPlanStep);

    double secondStep[] = {88,0};
    std::vector<double> secondPlanStep (secondStep, secondStep + sizeof(secondStep) / sizeof(double) );

    //plan.push_back(secondPlanStep);

    double thirdStep[] = {17};//, 4.79609};
    std::vector<double> thirdPlanStep (thirdStep, thirdStep + sizeof(thirdStep) / sizeof(double) );

    //plan.push_back(thirdPlanStep);

    double fourthStep[] = {32};
    std::vector<double> fourthPlanStep (fourthStep, fourthStep + sizeof(fourthStep) / sizeof(double) );

    //plan.push_back(fourthPlanStep);
    //std::clock_t    start = std::clock();
	 // your test

    std::cout << "Making estimator" << std::endl;

    std::cout << "Argument given: " << argv[1] << std::endl;
    PlanCoverageEstimator estimator = PlanCoverageEstimator(argv[1], cameraSpecs, false, nullptr, false, true);
    //std::cout << "First plan: " << std::endl;


    //std::cout << "Getting simple plans" << std::endl;
    std::vector<std::vector<std::vector <double> > > simplePlans = estimator.getSimplePlans();
    //std::cout << "Simple plan zero is: " << estimator.getSimplePlans() << std::endl;


    osg::Matrixd mat(1.0, 0.0, 0.0, 0.0,0.0, 0.0, -1.0, 0.0,0.0, 1.0, 0.0, 0.0,11.476787567138672, -5.896953582763672, -236.199951171875, 1.0);
    std::vector<std::vector<double> > vec = convertMatrixToNestedVector(mat);

//    for(int i = 0; i< simplePlans[0].size();i++){
//    	simplePlans[0][i].push_back(-1.0);
//    }
    std::vector<std::vector<std::vector<double> > > allSolutions;
    std::vector<double> testPlan{748, 458, 752, 213, 259, 754, 291, 751, 425, 30, 749, 724, 750, 507, 439, };
    std::vector<std::vector<double> > testPlanVector;
    for(auto part:testPlan){
    	std::vector<double> planPartVector{part};
    	testPlanVector.push_back(planPartVector);
    }

    allSolutions.push_back(plan);

    //std::cout <<  "Simple plans: " << simplePlans.size() << ", " << simplePlans[0].size() << std::endl;
//    std::vector<double> firstLoop{748};
//    std::vector<double> secondLoop{749};
//    std::vector<double> invalidLoop{750};
//    simplePlans[0].insert(simplePlans[0].begin(),secondLoop);
//    simplePlans[0].insert(simplePlans[0].begin(),firstLoop);
//    plan.push_back(firstLoop);


    //for(int i=0;i<simplePlans.size();i++){
    	//std::vector<std::vector <double> > simplePlan = simplePlans[i];
    //estimator.LocalSearch(simplePlans[3]);
    //std::cout << "Testing plan export." << std::endl;
    //std::vector<std::vector<double> > planPositions;
    //std::vector<std::vector<double> > planPoses;
    //estimator.interpretAndExportPlan(simplePlans[0], planPositions, planPoses);
    //std::cout << "Export result size: " << planPositions.size() << std::endl;

    std::vector<double> result = estimator.evaluatePlan(simplePlans[0],false,normal);//testPlanVector


    	//plan[1][1] = 0.2;
    	//std::vector<double> result2 = estimator.evaluatePlan(plan,true,true);

    	//estimator.updateMemoisedEdges(allSolutions);

    std::cout << "Estimator done. Result: " << result[0] << ", " << result[1] << std::endl;
    //}


	return 0;
}


