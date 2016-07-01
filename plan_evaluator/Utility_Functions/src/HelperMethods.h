/*
 * HelperMethods.h
 *
 *	Here, I will collect helper methods that are useful in many situations, such as print-methods, common calculations, etc.
 *
 *  Created on: May 28, 2015
 *      Author: kaiolae
 */

#ifndef HELPERMETHODS_H_
#define HELPERMETHODS_H_

#include <stddef.h>
#include <iostream>
#include <random>
#include <iterator>
#include <osg/Vec3>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/functional/hash.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <boost/filesystem/path.hpp>


//Helper methods used by hash functions below.
namespace {
	template<typename T>
	std::size_t make_hash(const T &v) {
		return std::hash<T>()(v);
	}

	void hash_combine(std::size_t &h, const std::size_t &v) {
		h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2);
	}

	template<typename T>
	struct hash_container {
		size_t operator()(const T &v) const {
			size_t h = 0;
			for (const auto &e : v) {
				hash_combine(h, make_hash(e));
			}
			return h;
		}
	};
}

//Hashing functions for some common containers
namespace std {
	template<typename T, typename U>
	struct hash<pair<T, U>> {
		size_t operator()(const pair<T, U> &v) const {
			size_t h = make_hash(v.first);
			hash_combine(h, make_hash(v.second));
			return h;
		}
	};

	template<typename... T>
	struct hash<vector<T...>> : hash_container<vector<T...>> {
	};

	template<typename... T>
	struct hash<map<T...>> : hash_container<map<T...>> {
};

//Hash for dynamic-bitset.
template<typename B, typename A>
struct hash<boost::dynamic_bitset<B, A> > {
	size_t operator()(const boost::dynamic_bitset<B, A> &bs) {
		//Slightly inefficient hashing method, but unfortunately, dynamic_bitset does not offer a nice alternative.
		std::vector<B, A> v;
		boost::to_block_range(bs, std::back_inserter(v));
		return boost::hash_range(v.begin(), v.end());
	}
};
}

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()


// return the full path of all files that have the specified extension
// in the specified directory and all subdirectories
void get_all(const boost::filesystem::path& root, const std::string& ext, std::vector<boost::filesystem::path>& ret);

///Helper method to print a vector.
template<typename T>
static std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) ;

///Helper method to print a vector of vectors.
template<typename T>
static std::ostream& operator<<(std::ostream& out, const std::vector<std::vector<T> >& grid);

///Helper method to print sizes of all subvectors in a vector of vectors.
template<typename T>
static std::string print_subvector_sizes(const std::vector<std::vector<T> >& inputVectors);

///Helper method to print a set of vectors.
template<typename T>
static std::ostream& operator<<(std::ostream& out, const std::set<std::vector<T> >& vecSet);

///Helper method to print an OSG vector.
std::ostream& operator<<(std::ostream& out, const osg::Vec3& vec);

///Helper method to print an array
template<typename T>
static void printArray(const T* array, int numelements);

/**
 * Converts a vector (or part of the vector) to a string, by converting each element and concatenating with commas.
 * Example: [1,2,3] -> "1,2,3"
 * @param inputVector The vector we want to convert. Has to be of a type that can be converted to string.
 * @param numElements How many of the elements (taken from beginning of vector) we want to be in the string.
 * @return The vector's content as a string
 */
template<typename T>
static std::string vectorToString(const std::vector<T>& inputVector, int numElements);

/**
 * Checks if the first given vector pareto dominates the other. Objective is assumed to be minimization.
 * Assumes the two have the same length, as dominance is only defined if the objectives considered are the same for both solutions.
 * @param firstVector
 * @param secondvector
 * @return
 */
bool vectorDominates(const std::vector<double>& firstVector, const std::vector<double>& secondvector);

/*
 * Returns true if file exists, false otherwise.
 */
bool does_file_exist(const char *fileName);

/*
 * Increments the input filename. If there is no number at the end of the filename (before the type-extension) already, a "1" is added.
 * If a number is there, it is incremented by 1.
 */
void increment_filename(std::string& input_file_path);

/**
 * Removes consecutive duplicated points from vectors. For instance, the vector
 * [1, 2, 2, 1, 3] turns into [1, 2, 1, 3].
 * @param plan The vector we are modifying.
 */
template<typename T>
static void removeConsecutiveDuplicatedPoints(std::vector<T>& v);

/**
 * Returns the ID of the first subvector that contains the given value. Optionally, one can pass the indexes
 * of subvectors one want to ignore (ignore_subvectors), in case one is not interested in looking through some of them.
 * @param vector_of_vectors The vector of vectors where we are searching for value.
 * @param value The value we wish to find
 * @param ignore_subvectors The indexes of subvectors one do not wish to look for the value in. For instance,
 * if this variable contains [1,2], subvector 1 and 2 in vector_of_vectors are never examined.
 * @throws runtime_error if the value is not found in any subvector
 */
template<typename T>
int first_subvector_that_contains(const std::vector<std::vector<T> >& vector_of_vectors, T value,
								  const std::set<int>&ignore_subvectors = std::set<int>());


/**
 * Straightforward greedy solver for the set cover problem (https://en.wikipedia.org/wiki/Set_cover_problem).
 * @param coverage_of_each_element Vector that describes the coverage of each set in the set cover problem.
 * Each bitset in the vector has 1's representing their coverage, and it is assumed that together, all
 * bitsets provide a full coverage (a bitset of all 1's).
 * @return ID's of a subset of all bitsets that together give a full coverage.
 */
std::vector<int> calculateSetCover(const std::vector<boost::dynamic_bitset<> >& coverage_of_each_element);


///Definitions for templated functions

template<typename T>
void removeConsecutiveDuplicatedPoints(std::vector<T>& v){
	typename std::vector<T>::iterator it;
	it = std::unique(v.begin(), v.end());
	v.resize(std::distance(v.begin(),it));
}


/**
 * Splits a string by a given delimiter character, returning each delimited part of the string in a vector.
 * @param inputString The string we would like to split.
 * @param splitCharacter The delimiting character for our split.
 * @return A vector containing the delimited parts of the string.
 */
const std::vector<std::string> splitString(const std::string& inputString, const char& splitCharacter);

template<typename T>
std::string vectorToString(const std::vector<T>& inputVector, int numElements){
	std::ostringstream oss;


	//typename std::ostream_iterator<int> itt;

	if (!inputVector.empty()&&numElements<=inputVector.size())
	{

		if(numElements==inputVector.size()) //The special case of wanting all the elements
		{
			// Convert all but the last element to avoid a trailing ","
			std::copy(inputVector.begin(), inputVector.end()-1, std::ostream_iterator<T>(oss, ","));
			// Now add the last element with no delimiter
			oss << inputVector.back();
		}
		else if(numElements==1){
			oss << inputVector.front();
		}
		else{
			std::copy(inputVector.begin(), inputVector.begin()+numElements, std::ostream_iterator<T>(oss, ","));
		}
	}
	return oss.str();
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::set<std::vector<T> >& vecSet){
	typedef typename std::vector<T> tVector;
	typedef typename std::set<std::vector<T> >::iterator setIt;
	out << "";
	for (setIt it = vecSet.begin(); it!= vecSet.end(); ++it)
	{
		tVector vec = *it;
	   for ( unsigned int j = 0; j < vec.size(); j++ )
	   {
	      out << vec[j] << ' ';
	   }
	   out << std::endl;
	}

	return out;
}

template<typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
    out << "[";
    size_t last = v.size() - 1;
    for(size_t i = 0; i < v.size(); ++i) {
        out << v[i];
        if (i != last)
            out << ", ";
    }
    out << "]";
    return out;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<std::vector<T> >& grid){
	out << "";
	for ( unsigned int i = 0; i < grid.size(); i++ )
	{
	   for ( unsigned int j = 0; j < grid[i].size(); j++ )
	   {
	      out << grid[i][j] << ", ";
	   }
	   if(grid[0].size()!=1){
		   out << std::endl; //Adding linebreak only if every row has multiple elements
	   }
	}

	return out;
}

//Transforms a map to a vector of the keys in the map
template <typename M, typename V>
void MapKeysToVec( const  M & map, V & vec) {
    for( typename M::const_iterator it = map.begin(); it != map.end(); ++it ) {
    	vec.push_back( it->first);
    }
}

template<typename T>
void printArray(const T* array, int numelements){
	for (int i = 0; i < numelements; i++){
	    std::cout << array[i];
		if(i!=numelements-1){
			std::cout << " ,";
		}
	}
}



template<typename T>
std::string print_subvector_sizes(const std::vector<std::vector<T> > &inputVectors) {
	std::string sizes = "";
	for(auto v:inputVectors){
		sizes += v.size() + ", ";
	}
	return sizes;
}

template<typename T>
int first_subvector_that_contains(const std::vector<std::vector<T> >& vector_of_vectors, T value,
								  const std::set<int>&ignore_subvectors/*=set<int>()*/){
	for(int vector_counter = 0; vector_counter < vector_of_vectors.size(); vector_counter++){
		//If a subvector is invalid, we do not check if it contains the element.
		if(ignore_subvectors.find(vector_counter) == ignore_subvectors.end()){
			const std::vector<T> subvector = vector_of_vectors[vector_counter];
			if(std::find(subvector.begin(), subvector.end(), value) != subvector.end()){
				return vector_counter;
			}
		}
	}

	throw std::runtime_error("None of the given vectors contain the desired value.");
}

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}



#endif /* HELPERMETHODS_H_ */
