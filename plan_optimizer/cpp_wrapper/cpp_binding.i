%module cpp_binding
%include "std_vector.i"
%include "std_string.i"
%include typemaps.i

%{
#define SWIG_FILE_WITH_INIT
#include "../../plan_evaluator/Evaluator/src/PlanCoverageEstimator.h"
%}


namespace std {
%template(Line)  vector < double >;
%template(Array) vector < vector < double> >;
%template() vector < vector < vector < double> > >;
%template(StringVector) vector < string >;
}

//enum defining the ways we may plot plans to screen
enum plotting_style {normal, circulating, image, nothing};

namespace evolutionary_inspection_plan_evaluation {
std::vector<std::vector<double> > viewMatrixSelector(std::string sceneFileName);


class PlanCoverageEstimator{
public:

PlanCoverageEstimator(const std::string& sceneFileName, const std::vector<double>& sensorSpecs, bool postProcessing = false,const std::vector<double>* startLocation = NULL, bool planLoopsAround = false,  bool printerFriendly = true);

std::vector<double> evaluatePlan(const std::vector<std::vector<double> >& plan, bool memoization, plotting_style how_to_plot, bool disableEnergyLimit = False);
int getNumberOfBoxes() const;
int updateMemoisedEdges(const std::vector<std::vector<std::vector<double> > >& allSolutions);
std::vector<std::vector<std::vector<double> > > getSimplePlans() const;
void storePlanImage(const std::vector<std::vector<double> >& plan, const std::vector<std::vector<double> >& viewMatrix, const std::string storagePath);
double getMaxAllowedEnergy() const;
//This is how SWIG turns a C++ return by reference into a Python multiple-argument return. Called in python like: [a, b] = interpretAndExportPlan(plan, [], [])
void interpretAndExportPlan(const std::vector<std::vector<double> >& plan, std::vector<std::vector<double> > &INOUT, std::vector<std::vector<double> > &INOUT);
};
}
