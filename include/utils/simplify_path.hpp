#ifndef IE_SIMPLIFY_HPP
#define IE_SIMPLIFY_HPP

#include "utils/Linear.hpp"

namespace ie {

/// \param start, end Start/end point of the path/sub-path.
/// \param point Intermediate point of the path/sub-path to consider.
/// \param sqSegLen Distance between start and end.
typedef float (
    *DeviationMetricCallback)(vec2 start, vec2 end, vec2 point, float sqSegLen);

/// \brief Iteratively stimulate Ramer-Douglas-Peucker algorithm.
///
/// \param resPoints Should be at least as large as the "points" passed in. Can be
/// 	identical to the "points" passed in.
///
/// \return 1 if successful, 0 otherwise. Notice that the content of "resPoints" is
/// undefined when failed.
///
/// \reference https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
int SimplifyPath(
    vec2 *points,
    unsigned int npoints,
    vec2 *resPoints,
    unsigned int *nResPoints,
    float epsilon,
    DeviationMetricCallback deviationMetric);

// Declare several metric function implementations.

extern DeviationMetricCallback PerpDistMetric;
extern DeviationMetricCallback ShortestDistMetric;

} // namespace ie

#endif // IE_SIMPLIFY_HPP
