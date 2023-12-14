#include <cerrno>
#include <cstdlib>
#include "utils/simplify_path.hpp"
#include "utils/Memory.hpp"
using namespace ie;

static inline float
getArea(vec2 start, vec2 end, vec2 mid)
{
	return start.x * (mid.y - end.y) + mid.x * (end.y - start.y) + end.x * (start.y - mid.y);
}

/// This one just returns the shortest distance to the infinite extension of the line
/// segment.
static float
perpendicularDistance(vec2 start, vec2 end, vec2 mid, float sqSegLen)
{
	return Pow2(getArea(start, end, mid)) / sqSegLen;
}

DeviationMetricCallback ie::PerpDistMetric = &perpendicularDistance;

static float
shortestDistanceToSegment(vec2 start, vec2 end, vec2 mid, float sqSegLen)
{
	float ax, ay, bx, by, cx, cy, ab, bc, area;

	ax = end.x - start.x;
	ay = end.y - start.y;
	bx = mid.x - start.x;
	by = mid.y - start.y;
	cx = mid.x - end.x;
	cy = mid.y - end.y;

	ab = ax * bx + ay * by;
	bc = bx * cx + by * cy;

	if (ab > 0 && bc < 0)
	{
		area = 0.5f * getArea(start, end, mid);
		return area * area / sqSegLen;
	}
	else
	{
		if (ab < 0 && bc < 0) return ax * ax + ay * ay;
		else return cx * cx + cy * cy;
	}
}

DeviationMetricCallback ie::ShortestDistMetric = &shortestDistanceToSegment;


/// These are out of order for the purposes of struct packing.
typedef struct
{
	vec2 *points;
	vec2 *resPoints;
	unsigned int npoints;
} subcall;

typedef enum
{
	DIVIDE,
	LINEARIZE,
	SOLVED
} rescode;

#define FAILURE 0
#define SUCCESS 1

static rescode solver(
    vec2 *points,
    int npoints,
    vec2 *resPoints,
    int *nResPoints,
    int *divisionIdx,
    float epsilon,
    DeviationMetricCallback deviationMetric);

/// The call stack will start able to hold this many calls and grow by this amount
/// whenever it needs to grow in size.
#define CALL_STACK_MAX_SIZE 2048

// callStackBase is a float pointer because realloc might move the base pointer.
static int
callStackPush(subcall **stack, int *capacity, int *size, subcall *call)
{
	// Check if the stack is full.
	if (*size >= *capacity)
	{
		// Grow the stack by a unit.
		*capacity += CALL_STACK_MAX_SIZE;
		*stack = static_cast<subcall *>(realloc(*stack, sizeof(subcall) * *capacity));
		if (errno || *stack == nullptr)
		{
			// The stack can't grow.
			return FAILURE;
		}
	}

	// Add the new call.
	(*stack)[*size] = *call;
	++(*size);

	return SUCCESS;
}

static int
callStackPop(subcall *stack, int *piNumCallsInStack, subcall *pPoppedCall)
{
	// Check if there is a call to pop.
	if (*piNumCallsInStack > 0)
	{
		--(*piNumCallsInStack);
		*pPoppedCall = stack[*piNumCallsInStack];
		return SUCCESS;
	}
	else
	{
		// There are no calls to pop.
		return FAILURE;
	}
}

// This is the cleanup macro for the CompactPath function.
// Free the only allocated memory and return the parameter
// as the return value.
#define COMPACT_PATH_RETURN(iReturnValue)                                                     \
   {                                                                                         \
	   _free(stack);                                                                          \
	   return iReturnValue;                                                                  \
   }

// This function iteratively simulates the recursive Ramer-Douglas-Peucker algorithm.
// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
// Please allocate the resultPointArray to be as large as the pointArray passed in.
// It would be a good idea to resize the allocated space for resultPointArray after this
// function returns using the value of pointsInResultPath.
// This algorithm works in-place. That is, you can use the same array for both pointArray
// and resultPointArray, keeping in mind that doing so will likely alter pointArray.
// Returns a true value (1) on successful completion, and returns a false value (0)
// otherwise. On failure, pointsInResultPath and the contents of resultPointArray are
// undefined.
int
ie::SimplifyPath(
    vec2 *points,
    unsigned int npoints,
    vec2 *resPoints,
    unsigned int *nResPoints,
    float epsilon,
    DeviationMetricCallback deviationMetric)
{
	subcall current, firstSubproblem, secondSubproblem;
	int divisionIdx; // Where should we split the problem into sub-problems?
	rescode code; // The status of the most recent sub-problem call
	int solvedPoints; // Keep track of how much of the result array is solved and in place.
	int validPoints; // Number of valid points in the result array after a sub-problem call
	int stackCap; // How many calls can the call stack hold right now?
	int stackSize; // How many calls are in the call stack right now?

	// Clear errno so that we can be sure that a nonzero value is caused by this function.
	errno = 0;

	// Copy the first point into the result. This can be done because its final location
	// is known (it will still be the first point), and it will certainly be in the final
	// array (it can never be removed). The compacter skips copying the first point of
	// each sub-problem because it is added as the last point of the sub-problem before it.
	// Copying the very first point is necessary because the leftmost sub-problem has no
	// prior sub-problem.
	*resPoints = *points;
	solvedPoints = 1;

	// Allocate a call stack.
	stackCap = CALL_STACK_MAX_SIZE;
	stackSize = 0;
	subcall *stack = (subcall *)_malloc(sizeof(subcall) * CALL_STACK_MAX_SIZE);

	if (errno || stack == nullptr)
	{
		COMPACT_PATH_RETURN(FAILURE);
	}

	// Set up the first instance of the problem, representing the whole problem.
	current.points = points;
	current.resPoints = resPoints;
	current.npoints = npoints;

	// Add the first instance to the stack
	if (!callStackPush(&stack, &stackCap, &stackSize, &current))
	{
		COMPACT_PATH_RETURN(FAILURE);
	}

	// As long as there are calls on the stack, pop one and process it.
	while (stackSize > 0)
	{
		if (!callStackPop(stack, &stackSize, &current))
		{
			COMPACT_PATH_RETURN(FAILURE);
		}

		code = solver(
		    current.points, current.npoints, current.resPoints, &validPoints, &divisionIdx,
		    epsilon, deviationMetric);

		if (code == DIVIDE)
		{
			if (divisionIdx <= 0 || (unsigned int)divisionIdx >= current.npoints)
			{
				COMPACT_PATH_RETURN(FAILURE);
			}
			else
			{
				// Create two new subproblems and push them.
				// It's important that the way that results are copied is compatible with
				// the order in which the subproblem calls are pushed to the stack. This
				// function is left-recursive. It performs left-side subproblems before
				// right-side subproblems. This means that the left-side subproblem needs
				// to be pushed to the stack last, so that it is popped back out first.

				secondSubproblem.points = current.points + divisionIdx;
				secondSubproblem.resPoints = current.resPoints + divisionIdx;
				secondSubproblem.npoints = current.npoints - divisionIdx;
				if (!callStackPush(&stack, &stackCap, &stackSize, &secondSubproblem))
				{
					COMPACT_PATH_RETURN(FAILURE);
				}

				firstSubproblem.points = current.points;
				firstSubproblem.resPoints = current.resPoints;
				firstSubproblem.npoints = divisionIdx + 1;
				if (!callStackPush(&stack, &stackCap, &stackSize, &firstSubproblem))
				{
					COMPACT_PATH_RETURN(FAILURE);
				}
			}
		}
		else if (code == LINEARIZE || code == SOLVED)
		{
			// Copy the results to their final destination in the result array.

			// Always skip copying the first point.
			++current.resPoints;
			--validPoints;

			// There's a good chance that the memory regions will overlap at some point.
			memmove(resPoints + solvedPoints, current.resPoints, sizeof(vec2) * validPoints);

			solvedPoints += validPoints;
		}
		else
		{
			// Bad result code.
			COMPACT_PATH_RETURN(FAILURE);
		}
	}

	*nResPoints = solvedPoints;

	COMPACT_PATH_RETURN(SUCCESS);
}

/// If the result is DIVIDE, it means that the algorithm needs to
/// divide the problem into two smaller subproblems. In this case, divisionIndex is set to
/// the index of the point that should be the end point of the first subproblem and the
/// start point of the second subproblem. pointsInCurrentPath is not set, because its
/// value is not yet known. If the result is LINEARIZE, it means that all the intermediate
/// points in the subproblem were removed. In this case, divisionIndex is not set. If the
/// result is SOLVED, it means that the algorithm does not need to do any further work on
/// the subproblem, because it is already solved. In this case, divisionIndex is not set.
static rescode
solver(
    vec2 *points,
    int npoints,
    vec2 *resPoints,
    int *nResPoints,
    int *divisionIdx,
    float epsilon,
    DeviationMetricCallback deviationMetric)
{
	float sqSegLen, sqDev, sqMaxDev, dx, dy;
	int i, maxPointIdx;

	// If there are fewer than three points provided, the problem is solved already.
	if (npoints < 3)
	{
		// Just copy pointArray into resultPointArray.
		if (npoints > 0)
		{
			memcpy(resPoints, points, sizeof(vec2) * npoints);
		}
		*nResPoints = npoints;
		return SOLVED;
	}

	sqMaxDev = 0;

	dx = points[npoints - 1].x - points[0].x;
	dy = points[npoints - 1].y - points[0].y;
	sqSegLen = dx * dx + dy * dy;

	for (i = 1; i < npoints - 1; ++i)
	{
		sqDev = deviationMetric(points[0], points[npoints - 1], points[i], sqSegLen);

		if (sqDev > sqMaxDev)
		{
			maxPointIdx = i;
			sqMaxDev = sqDev;
		}
	}

	if (sqMaxDev < epsilon * epsilon)
	{
		// Linearize the points in the subproblem.
		// To do this, we just copy the first and last points to the result array.
		resPoints[0] = points[0];
		resPoints[1] = points[npoints - 1];
		*nResPoints = 2;
		return LINEARIZE;
	}
	else
	{
		// Split the subproblem.
		*divisionIdx = maxPointIdx;
		return DIVIDE;
	}
}
