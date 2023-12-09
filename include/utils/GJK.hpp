#ifndef IE_GJK_HPP
#define IE_GJK_HPP

#include <cstdint>
#include "utils/AABB.hpp"
#include "utils/Linear.hpp"


namespace ie {
namespace gjk {

const int MAX_POLY_VERTS = 8;

/// chosen to be numerically significant, but visually insignificant. In meters.
const float LINEAR_SLOP = 0.005f;

const float HUGE = 100000.f;

// ██████╗      ██╗██╗  ██╗
//██╔════╝      ██║██║ ██╔╝
//██║  ███╗     ██║█████╔╝
//██║   ██║██   ██║██╔═██╗
//╚██████╔╝╚█████╔╝██║  ██╗
// ╚═════╝  ╚════╝ ╚═╝  ╚═╝

//! \defgroup GJK
//! @{

/// Ray-cast input data. The ray extends from p1 to p1 + maxFraction * (p2 - p1).
struct RayCast
{
	vec2 p1, p2;
	float maxFraction;
	float radius;

	RayCast(vec2 start, vec2 end, float maxDist = HUGE, float r = 0.f);
};

/// Ray-cast output data.
/// The ray hits at p1 + fraction * (p2 - p1), where p1 and p2 come from RayCast.
struct RayHit
{
	vec2 normal{0.f, 0.f};
	vec2 point{0.f, 0.f};
	float fraction{0.f};
	int32_t iterations{0};
	bool hit{false};
};

/// Rotation
struct Rot2d
{
	/// Sine and cosine
	float s{0.f}, c{1.f};

	Rot2d() = default;
	constexpr Rot2d(float sv, float cv) : s(sv), c(cv) { }

	explicit Rot2d(float angle)
	{
		s = Sin(angle);
		c = Cos(angle);
	}

	void Set(float angle)
	{
		s = Sin(angle);
		c = Cos(angle);
	}

	void SetIdentity()
	{
		s = 0.0f;
		c = 1.0f;
	}

	float GetAngle() const { return Atan2(s, c); }
};

static constexpr Rot2d rot2d_identity = {0.f, 1.f};

/// A 2D rigid transform
struct Xf2d
{
	vec2 p{0.f, 0.f};
	Rot2d q;

	Xf2d() = default;
	constexpr Xf2d(const vec2& v, const Rot2d& r) : p(v), q(r) { }

	void SetIdentity()
	{
		p = {0.f, 0.f};
		q.SetIdentity();
	}
};

static constexpr Xf2d xf2d_identity = {vec2{}, rot2d_identity};

struct SegmentDistanceResult
{
	vec2 closest1{};
	vec2 closest2{};
	float fraction1;
	float fraction2;
	float distanceSquared;

	SegmentDistanceResult() : fraction1(0.f), fraction2(0.f), distanceSquared(0.f) { }
};

/// A distance proxy is used by the GJK algorithm.
/// It encapsulates any shape.
struct DistanceProxy
{
	vec2 vertices[MAX_POLY_VERTS];
	int32_t count;
	float radius;
};

/// Used to warm start ShapeDistance.
/// Set count to zero on first call.
struct DistanceCache
{
	float metric; ///< length or area
	uint16_t count{0};
	uint8_t indexA[3]; ///< vertices on shape A
	uint8_t indexB[3]; ///< vertices on shape B
};

/// Input for b2Distance.
/// You have to option to use the shape radii
/// in the computation. Even
struct DistanceInput
{
	DistanceProxy proxyA;
	DistanceProxy proxyB;
	Xf2d transformA;
	Xf2d transformB;
	bool useRadii;
};

/// Output for b2Distance.
struct DistanceOutput
{
	vec2 pointA; ///< closest point on shapeA
	vec2 pointB; ///< closest point on shapeB
	float distance{0.f};
	int32_t iterations{0}; ///< number of GJK iterations used
};

/// Input parameters for b2ShapeCast
struct ShapeCastInput
{
	DistanceProxy proxyA;
	DistanceProxy proxyB;
	Xf2d transformA;
	Xf2d transformB;
	vec2 translationB;
	float maxFraction;
};

/// This describes the motion of a body/shape for TOI computation. Shapes are defined with respect to the body origin,
/// which may not coincide with the center of mass. However, to support dynamics we must interpolate the center of mass
/// position.
struct Sweep
{
	vec2 localCenter; ///< local center of mass position
	vec2 c1, c2; ///< center world positions
	float a1, a2; ///< world angles
};

/// Input parameters for TimeOfImpact
struct TOIInput
{
	DistanceProxy proxyA;
	DistanceProxy proxyB;
	Sweep sweepA;
	Sweep sweepB;

	// defines sweep interval [0, tMax]
	float tMax;
};

enum class TOIState
{
	kUnknown,
	kFailed,
	kOverlapped,
	kHit,
	kSeparated
};

/// Output parameters for b2TimeOfImpact.
struct TOIOutput
{
	TOIState state;
	float t;
};

/// A solid circle
struct Circle
{
	vec2 point;
	float radius;
};

/// A solid capsule
struct Capsule
{
	vec2 point1, point2;
	float radius;
};

/// A solid convex polygon. It is assumed that the interior of the polygon is to
/// the left of each edge.
/// Polygons have a maximum number of vertices equal to b2_maxPolygonVertices.
/// In most cases you should not need many vertices for a convex polygon.
struct Polygon
{
	vec2 vertices[MAX_POLY_VERTS];
	vec2 normals[MAX_POLY_VERTS];
	float radius;
	int32_t count;
};

/// Convex hull used for polygon collision
struct Hull
{
	vec2 points[MAX_POLY_VERTS];
	int32_t count;
};

/// A line segment with two-sided collision.
struct Segment
{
	vec2 p1, p2;

	Segment() = default;
	constexpr Segment(const vec2& p1v, const vec2& p2v) : p1(p1v), p2(p2v) { }

	Segment operator+(const Segment& rhs) const
	{
		return {AddV(p1, rhs.p1), AddV(p2, rhs.p2)};
	}

	Segment operator-(const Segment& rhs) const
	{
		return {SubV(p1, rhs.p1), SubV(p2, rhs.p2)};
	}

	vec2 Direction(bool normalize = true) const
	{
		vec2 d = SubV(p2, p1);
		if (normalize) { return NormV(d); }
		else { return d; }
	}

	vec2 Normal() const
	{
		const vec2 d = Direction();
		// return the direction vector
		// rotated by 90 degrees counter-clockwise
		return {-d.y, d.x};
	}
};

bool SegmentIntersection(const Segment& a, const Segment& b, bool infiniteLines, vec2& out);

/// A smooth line segment with one-sided collision. Only collides on the right side.
/// Normally these are generated from a chain shape.
/// ghost1 -> point1 -> point2 -> ghost2
/// This is only relevant for contact manifolds, otherwise use a regular segment.
struct SmoothSegment
{
	vec2 ghost1; ///< The tail ghost vertex
	vec2 point1, point2; ///< The line segment
	vec2 ghost2; ///< The head ghost vertex
};

// Inverse rotate a vector
static inline vec2
InvRotateVector(Rot2d q, vec2 v)
{
	return {q.c * v.x + q.s * v.y, -q.s * v.x + q.c * v.y};
}

// Transform a point (e.g. local space to world space)
static inline vec2
TransformPoint(Xf2d xf, const vec2 p)
{
	float x = (xf.q.c * p.x - xf.q.s * p.y) + xf.p.x;
	float y = (xf.q.s * p.x + xf.q.c * p.y) + xf.p.y;

	return {x, y};
}

// Rotate a vector
static inline vec2
RotateVector(Rot2d q, vec2 v)
{
	return {q.c * v.x - q.s * v.y, q.s * v.x + q.c * v.y};
}

// Inverse transform a point (e.g. world space to local space)
static inline vec2
InvTransformPoint(Xf2d xf, const vec2 p)
{
	float vx = p.x - xf.p.x;
	float vy = p.y - xf.p.y;
	return {xf.q.c * vx + xf.q.s * vy, -xf.q.s * vx + xf.q.c * vy};
}

/// Transpose multiply two rotations: qT * r
static inline Rot2d
InvMulRot(Rot2d q, Rot2d r)
{
	// [ qc qs] * [rc -rs] = [qc*rc+qs*rs -qc*rs+qs*rc]
	// [-qs qc]   [rs  rc]   [-qs*rc+qc*rs qs*rs+qc*rc]
	// s = qc * rs - qs * rc
	// c = qc * rc + qs * rs
	Rot2d qr;
	qr.s = q.c * r.s - q.s * r.c;
	qr.c = q.c * r.c + q.s * r.s;
	return qr;
}

// v2 = A.q' * (B.q * v1 + B.p - A.p)
//    = A.q' * B.q * v1 + A.q' * (B.p - A.p)
static inline Xf2d
InvMulTransforms(Xf2d A, Xf2d B)
{
	Xf2d C;
	C.q = InvMulRot(A.q, B.q);
	C.p = InvRotateVector(A.q, SubV(B.p, A.p));
	return C;
}

/// Compute the distance between two line segments, clamping at the end points if needed.
SegmentDistanceResult SegmentDistance(vec2 p1, vec2 q1, vec2 p2, vec2 q2);

/// Compute the closest points between two shapes. Supports any combination of:
/// Circle, Polygon, b2EdgeShape. The simplex cache is input/output.
/// On the first call set b2SimplexCache.count to zero.
DistanceOutput ShapeDistance(DistanceCache* cache, const DistanceInput* input);

/// Perform a linear shape cast of shape B moving and shape A fixed. Determines the hit point, normal, and translation fraction.
/// @returns true if hit, false if there is no hit or an initial overlap
RayHit ShapeCast(const ShapeCastInput* input);

DistanceProxy MakeProxy(const vec2* vertices, int32_t count, float radius);

Xf2d GetSweepTransform(const Sweep* sweep, float time);

/// Compute the upper bound on time before two shapes penetrate. Time is represented as
/// a fraction between [0,tMax]. This uses a swept separating axis and may miss some intermediate,
/// non-tunneling collisions. If you change the time interval, you should call this function
/// again.
TOIOutput TimeOfImpact(const TOIInput* input);

/// Test a point in local space
bool PointInCircle(vec2 point, const Circle* shape);
bool PointInCapsule(vec2 point, const Capsule* shape);
bool PointInPolygon(vec2 point, const Polygon* shape);

/// Helper functions to make convex polygons
Polygon MakePolygon(const Hull* hull, float radius);
Polygon MakePolygon(const vec2* vertices, int count, float radius);
Polygon MakeSquare(float h);
Polygon MakeBox(float hx, float hy);
Polygon MakeRoundedBox(float hx, float hy, float radius);
Polygon MakeOffsetBox(float hx, float hy, vec2 center, float angle);
Polygon MakeCapsule(vec2 p1, vec2 p2, float radius);
Polygon MakeAABB(const Box& aabb);

// Ray cast versus shape in shape local space. Initial overlap is treated as a miss.
RayHit RayCastCircle(const RayCast* input, const Circle* shape);
RayHit RayCastCapsule(const RayCast* input, const Capsule* shape);
RayHit RayCastSegment(const RayCast* input, const Segment* shape);
RayHit RayCastPolygon(const RayCast* input, const Polygon* shape);

/// These compute the bounding box in world space
Box ComputeCircleAABB(const Circle* shape, const Xf2d& xf);
Box ComputeCapsuleAABB(const Capsule* shape, const Xf2d& xf);
Box ComputePolygonAABB(const Polygon* shape, const Xf2d& xf);
Box ComputeSegmentAABB(const Segment* shape, const Xf2d& xf);

/// Compute the convex hull of a set of points. Returns an empty hull if it fails.
/// Some failure cases:
/// - all points very close together
/// - all points on a line
/// - less than 3 points
/// - more than MAX_POLY_VERTS points
/// This welds close points and removes collinear points.
Hull ComputeHull(const vec2* points, int32_t count);

/// This determines if a hull is valid. Checks for:
/// - convexity
/// - collinear points
/// This is expensive and should not be called at runtime.
bool ValidateHull(const Hull* hull);

/// A manifold point is a contact point belonging to a contact
/// manifold. It holds details related to the geometry and dynamics
/// of the contact points.
struct ManifoldPoint
{
	vec2 point; ///< world coordinates of contact point
	float separation; ///< the separation of the contact point, negative if penetrating
	float normalImpulse; ///< the non-penetration impulse
	float tangentImpulse; ///< the friction impulse
	uint16_t id; ///< uniquely identifies a contact point between two shapes
	bool persisted; ///< did this contact point exist the previous step?
};

/// Contact manifold convex shapes.
struct Manifold
{
	ManifoldPoint points[2];
	vec2 normal{0.f, 0.f};
	int32_t pointCount{0};
};

// clang-format off
Manifold GJK_Circles(const Circle* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd);
Manifold GJK_Capsule2Circle(const Capsule* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd);
Manifold GJK_Seg2Circle(const Segment* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd);
Manifold GJK_Poly2Circle(const Polygon* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd);
Manifold GJK_Capsules(const Capsule* A, Xf2d xfA, const Capsule* B, Xf2d xfB, float maxd, DistanceCache* cache);
Manifold GJK_Seg2Capsule(const Segment* A, Xf2d xfA, const Capsule* B, Xf2d xfB, float maxd, DistanceCache* cache);
Manifold GJK_Poly2Capsule(const Polygon* A, Xf2d xfA, const Capsule* B, Xf2d xfB, float maxd, DistanceCache* cache);
Manifold GJK_Polygons(const Polygon* A, Xf2d xfA, const Polygon* B, Xf2d xfB, float maxd, DistanceCache* cache);
Manifold GJK_Seg2Poly(const Segment* A, Xf2d xfA, const Polygon* B, Xf2d xfB, float maxd, DistanceCache* cache);
// clang-format on

/// SAT.
bool ShapeIntersect(
    const vec2* a,
    uint32_t na,
    const vec2* b,
    uint32_t nb,
    const Xf2d& xfa = xf2d_identity,
    const Xf2d& xfb = xf2d_identity);
bool ShapeIntersect(
    const Polygon& a,
    const Polygon& b,
    const Xf2d& xfa = xf2d_identity,
    const Xf2d& xfb = xf2d_identity);

bool RayIntersect(
    const vec2& as,
    const vec2& ad,
    const vec2& bs,
    const vec2& bd,
    vec2& intersection);


// From Real-time Collision Detection, p179.
bool RaycastAABB(const Box& a, const vec2& p1, const vec2& p2, RayHit* output);

//! @}
} // namespace gjk
} // namespace ie

#endif // IE_GJK_HPP
