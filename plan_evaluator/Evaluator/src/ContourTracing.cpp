/**
 * This algorithm is called Moore Neighbor Tracing
 * An explanation of the algorithm can be found here:
 * http://www.thebigblob.com/moore-neighbor-tracing-algorithm-in-c/
 *
 * @author Erik Smistad <smistad@idi.ntnu.no>
 */

#include "ContourTracing.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>


/**
 * Helper method converting a 2D vector to an array.
 * @param inputVector The input vector we wish to covert. The vector can be of any type (content-wise), but has to be 2D.
 * @return An array with the same contents ast the 2D vector.
 */

template<class T>
T * convert2DVectorToArray(std::vector<std::vector<T> >* inputVector){
	int width = inputVector->size();
	std::vector<T> row = (*inputVector)[0];
	int height = row.size();
	T * array = (T *)malloc(sizeof(T) * (height) * (width));

	for(int y = 0; y < (height); y ++)
	{
		for(int x = 0; x < (width); x ++)
		{
			array[x + y*(width)] = (*inputVector)[x][y];
		}
	}
	return array;
}


/**
 * Helper method which pads an image represented by a 2D bool array with
 * extra values around each edge (an extra "frame" around the image).
 * @param image The image to be padded
 * @param width Width of image
 * @param height Height of image
 * @param paddingType The value we wish to give to the newly appended (padded) frame.
 * @return Padded image
 */
bool * padImage(bool * image, int width, int height, bool paddingType)
{
	bool * paddedImage = (bool *)malloc(sizeof(bool) * (height+2) * (width+2));
	for(int x = 0; x < width+2; x ++)
	{
		for(int y = 0; y < height+2; y ++)
		{
			if(x == 0 || y == 0 || x == width+1 || y == height+1)
			{
				paddedImage[x + y*(width+2)] = paddingType;
			}
			else
			{
				paddedImage[x+y*(width+2)] = image[x-1 + (y-1)*width];
			}
		}
	}
	return paddedImage;
}


std::vector<std::pair<int,int> > mooreNeighborTracing(std::vector<std::vector<bool> > * imageVector)
{
	int width = imageVector->size();
	std::vector<bool> row = (*imageVector)[0];
	int height = row.size();

	std::vector<std::pair<int,int> > visitedNeighbors;
	bool * image = convert2DVectorToArray(imageVector);

	//Keeping printout as it may be useful for testing.
/*	std::cout << "Input image is: " << std::endl;
	int rc = 0;
	for(int x = 0; x < width; x ++)
	{
		std::cout << rc << ": ";
		for(int y = 0; y < height; y ++)
		{

			if(image[x+y*width])
				std::cout << "1";
			else
				std::cout << "0";
		}
		rc++;
		std::cout << std::endl;
	}
	std::cout << "Calculated width and height are " << width << ", " << height << std::endl;
*/
	bool inside = false;
	int pos = 0;

	// Need to start by padding the image by 1 bool
	bool * paddedImage = padImage(image, width, height, false);

	// Allocate new image as a 1D array
	bool * borderImage = (bool *)malloc(sizeof(bool) * (height+2) * (width+2));

	// Set entire image to false
	for(int y = 0; y < (height+2); y ++)
	{
		for(int x = 0; x < (width+2); x ++)
		{
			borderImage[x + y*(width+2)] = false;
		}
	}

	///In the trace, pixels marked "true" represent edges, "false" represent everything else.
	///The result of the trace is stored into the vector visitedNeighbors, in the order the pixels were traced.
	for(int y = 0; y < (height+2); y ++)
	{
		for(int x = 0; x < (width+2); x ++)
		{
			pos = x + y*(width+2);

			// Scan for true pixel
			if(borderImage[pos] == true && !inside)		// Entering an already discovered border
			{
				inside = false;
			}
			else if(paddedImage[pos] == true && inside)	// Already discovered border point
			{
				continue;
			}
			else if(paddedImage[pos] == false && inside)	// Leaving a border
			{
				inside = true;
			}
			else if(paddedImage[pos] == true && !inside)	// Undiscovered border point
			{

				visitedNeighbors.push_back(std::pair<int,int>(pos/(width+2)-1,pos%(width+2)-1));

				borderImage[pos] = true; 	// Mark the start pixel
				int checkLocationNr = 1;	// The neighbor number of the location we want to check for a new border point
				int checkPosition;			// The corresponding absolute array address of checkLocationNr
				int newCheckLocationNr; 	// Variable that holds the neighborhood position we want to check if we find a new border at checkLocationNr
				int startPos = pos;			// Set start position
				int counter = 0; 			// Counter is used for the jacobi stop criterion
				int counter2 = 0; 			// Counter2 is used to determine if the point we have discovered is one single point

				// Defines the neighborhood offset position from current position and the neighborhood
				// position we want to check next if we find a new border at checkLocationNr
				int neighborhood[8][2] = {
						{-1,7},
						{-3-width,7},
						{-width-2,1},
						{-1-width,1},
						{1,3},
						{3+width,3},
						{width+2,5},
						{1+width,5}
					};
				// Trace around the neighborhood
				while(true)
				{
					checkPosition = pos + neighborhood[checkLocationNr-1][0];
					newCheckLocationNr = neighborhood[checkLocationNr-1][1];

					if(paddedImage[checkPosition] == true) // Next border point found
					{
						visitedNeighbors.push_back(std::pair<int,int>(checkPosition/(width+2)-1,checkPosition%(width+2)-1));
						if(checkPosition == startPos)
						{
							counter ++;

							// Stopping criterion (jacob)
							if(newCheckLocationNr == 1 || counter >= 3)
							{
								// Close loop
								inside = true; // Since we are starting the search at were we first started we must set inside to false
								break;
							}
						}

						checkLocationNr = newCheckLocationNr; // Update which neighborhood position we should check next
						pos = checkPosition;
						counter2 = 0; 						// Reset the counter that keeps track of how many neighbors we have visited
						borderImage[checkPosition] = true; // Set the border pixel
					}
					else
					{
						// Rotate clockwise in the neighborhood
						checkLocationNr = 1 + (checkLocationNr % 8);
						if(counter2 > 8)
						{
							// If counter2 is above 8 we have traced around the neighborhood and
							// therefor the border is a single true pixel and we can exfalseit
							counter2 = 0;
							break;
						}
						else
						{
							counter2 ++;
						}
					}
				}
			}
		}
	}

	// Keeping the printouts below commented, as it is useful for testing/debugging.
	//The first printed matrix is the originally calculated edge image, and the second is the one I use later.
	//If everything works correctly, they should be EQUAL!
//	bool * clippedBorderImage = (bool *)malloc(sizeof(bool) * (height) * (width));
//	std::cout << "The border image looks like this: " << std::endl;
//	int rowCounter = 0;
//	for(int x = 0; x < width; x ++)
//	{
//		std::cout << rowCounter << ":";
//		for(int y = 0; y < height; y ++)
//		{
//			clippedBorderImage[x+y*width] = borderImage[x+1+(y+1)*(width+2)];
//
//			if(clippedBorderImage[x+y*width]){
//				std::cout << "1";
//			}else{
//				std::cout << "0";
//			}
//		}
//		rowCounter+=1;
//		std::cout << std::endl;
//	}

/*	std::cout << "Result: " << std::endl;
	int rowCounter2 = 0;
	for(int x2 = 0; x2 < width; x2 ++)
	{
		std::cout << rowCounter2 << ":";
		for(int y2 = 0; y2 < height; y2 ++)
		{
			//clippedBorderImage[x2+y2*width] = borderImage[x2+1+(y2+1)*(width+2)];

			if(std::find(visitedNeighbors.begin(), visitedNeighbors.end(),std::pair<int,int>(y2,x2))==visitedNeighbors.end()){
				std::cout << "1";
			}else{
				std::cout << "0";
			}
		}
		rowCounter2+=1;
		std::cout << std::endl;
	}*/
	return visitedNeighbors;
}
