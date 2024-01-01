#ifndef CANDYBOX_SIMPLIFY_HPP__
#define CANDYBOX_SIMPLIFY_HPP__

#include <glm/vec2.hpp>

namespace candybox {

/// \param start, end Start/end point of the path/sub-path.
/// \param point Intermediate point of the path/sub-path to consider.
/// \param sqSegLen Distance between start and end.
typedef float (
    *DeviationMetricCallback)(glm::vec2 start, glm::vec2 end, glm::vec2 point, float sqSegLen);

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
    glm::vec2 *points,
    unsigned int npoints,
    glm::vec2 *resPoints,
    unsigned int *nResPoints,
    float epsilon,
    DeviationMetricCallback deviationMetric);

// Declare several metric function implementations.

extern DeviationMetricCallback PerpDistMetric;
extern DeviationMetricCallback ShortestDistMetric;

} // namespace candybox

#endif // CANDYBOX_SIMPLIFY_HPP__
