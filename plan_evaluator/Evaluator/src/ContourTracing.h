/**
 * This algorithm is called Moore Neighbor Tracing
 * An explanation of the algorithm can be found here:
 * http://www.thebigblob.com/moore-neighbor-tracing-algorithm-in-c/
 *
 * It is used to generate baseline inspection plans, which "trace" their way around the
 * 3D structures we want to inspect.
 *
 * @author Erik Smistad <smistad@idi.ntnu.no>. Adapted by Kai Olav Ellefsen
 */

#ifndef CONTOURTRACING_H_
#define CONTOURTRACING_H_

#include <utility>
#include <vector>

/**
 * This algorithm is called Moore Neighbor Tracing
 * An explanation of the algorithm can be found here:
 * http://www.imageprocessingplace.com/downloads_V3/root_downloads/tutorials/contour_tracing_Abeer_George_Ghuneim/moore.html
 * @param imageVector The image we wish to border trace
 * @return The coordinates (x,y) of all border pixels, in the order they were traced.
 */
std::vector<std::pair<int,int> > mooreNeighborTracing(std::vector<std::vector<bool> > * imageVector);

#endif /* CONTOURTRACING_H_ */
