/*
 * HelperMethods.h
 *
 *	Here, I will collect helper methods that are useful in many situations, such as print-methods, common calculations, etc.
 *
 *  Created on: May 28, 2015
 *      Author: kaiolae
 */

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <fstream>
#include "HelperMethods.h"

namespace fs = ::boost::filesystem;


void get_all(const fs::path& root, const std::string& ext, std::vector<fs::path>& ret)
{
	if(!fs::exists(root) || !fs::is_directory(root)) return;

	fs::recursive_directory_iterator it(root);
	fs::recursive_directory_iterator endit;

	while(it != endit)
	{
		if(fs::is_regular_file(*it) && it->path().extension() == ext) ret.push_back(it->path());
		++it;

	}

}

std::ostream& operator<<(std::ostream& out, const osg::Vec3& vec)
{
   for (int i = 0; i < 3; ++i)
   {
      out << "[" << vec[i] << "]";
   }
   out << std::endl;
   return out;
}

const std::vector<std::string> splitString(const std::string& inputString, const char& splitCharacter)
{
	std::string buff{""};
	std::vector<std::string> v;

	for(auto letter:inputString)
	{
		if(letter != splitCharacter) buff+=letter; else
		if(letter == splitCharacter && buff != "") { v.push_back(buff); buff = ""; }
	}
	if(buff != "") v.push_back(buff);

	return v;
}


bool vectorDominates(const std::vector<double>& firstVector, const std::vector<double>& secondvector){

	bool someElementInFirstDominatesSecond = false;
	for(unsigned int i=0; i<firstVector.size(); i++){
		if(firstVector[i] > secondvector[i]){
			return false; //If any element in first is worse than in second, it doesn't dominate.
		}
		if(firstVector[i] < secondvector[i]){
			someElementInFirstDominatesSecond = true;
		}
	}
	//Returns true if some element is larger in the first vector, and none is smaller.
	return someElementInFirstDominatesSecond;
}

std::vector<int> calculateSetCover(const std::vector<boost::dynamic_bitset<> >& coverage_of_each_element){
	//ID's of the minimum combination of elements in the input required for full coverage
	std::vector<int> minimum_elements_required_for_full_coverage;

	boost::dynamic_bitset<> incompletely_covered_elements(coverage_of_each_element.begin()->size());
	incompletely_covered_elements.flip(); //Making all entries 1 - all are incomplete.

	//Iterating until all elements are covered
	while (incompletely_covered_elements.count() != 0) {
		//Find element in coverage_of_each_element that covers the largest number of incomplete primitives.
		size_t maxCoveredElements = 0;
		int bestElementID = 0;

		int counter = 0;
		for (auto &current_coverage: coverage_of_each_element) {

			if(minimum_elements_required_for_full_coverage.size() == coverage_of_each_element.size()){
				//If we added all sets and didn't yet finish, the input was incomplete.
				throw std::logic_error("The input sets do not together provide a complete coverage. Set cover unsolvable.");
			}

			//Skipping elements already added to covering set.
			if(std::find(minimum_elements_required_for_full_coverage.begin(), minimum_elements_required_for_full_coverage.end(),
			counter)!=minimum_elements_required_for_full_coverage.end()){
				counter++;
				continue;
			}

			//AND-ing. Resulting bitset has 1's for all elements still not covered.
			std::cout << "Current coverage is " << current_coverage.count() << std::endl;
			boost::dynamic_bitset<> new_unobserved_primitives = incompletely_covered_elements & current_coverage;
			if (new_unobserved_primitives.count() > maxCoveredElements) {
				std::cout << "Added to MAX" << std::endl;
				maxCoveredElements = new_unobserved_primitives.count();
				bestElementID = counter;
			}
			counter++;
		}

		//Update incompletePrimitives, and insert the best element in the setCover.
		boost::dynamic_bitset<> coveredPrimitives = coverage_of_each_element[bestElementID];
		std::cout << "Coverage of added element: " << coveredPrimitives.count() << std::endl;
		//ANDing with the inverse of the covered primitives. This makes all covered primitives zero, like we want.
		incompletely_covered_elements &= ~coveredPrimitives;
		minimum_elements_required_for_full_coverage.push_back(bestElementID);


		std::cout << "added element " << bestElementID << " to set cover." << std::endl;
		std::cout << "Num incomplete primitives: " << incompletely_covered_elements.count() << std::endl;

	}

	std::cout << "Set cover complete. Size is " << minimum_elements_required_for_full_coverage.size() << std::endl;
	return minimum_elements_required_for_full_coverage;

}