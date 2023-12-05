#include "utils/GJK.hpp"
#include "utils/Macros.hpp"

using namespace ie;

#define RESTRICT

gjk::Xf2d
ie::gjk::GetSweepTransform(const Sweep* sweep, float time)
{
	// https://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/
	Xf2d xf;
	xf.p = AddV(MulSV(1.0f - time, sweep->c1), MulSV(time, sweep->c2));
	float angle = (1.0f - time) * sweep->a1 + time * sweep->a2;
	xf.q.Set(angle);

	// Shift to origin
	xf.p = SubV(xf.p, RotateVector(xf.q, sweep->localCenter));
	return xf;
}

/// Follows Ericson 5.1.9 Closest Points of Two Line Segments
gjk::SegmentDistanceResult
ie::gjk::SegmentDistance(vec2 p1, vec2 q1, vec2 p2, vec2 q2)
{
	SegmentDistanceResult result{};

	vec2 d1 = SubV(q1, p1);
	vec2 d2 = SubV(q2, p2);
	vec2 r = SubV(p1, p2);
	float dd1 = DotV(d1, d1);
	float dd2 = DotV(d2, d2);
	float rd2 = DotV(r, d2);
	float rd1 = DotV(r, d1);

	const float epsSqr = FLT_EPSILON * FLT_EPSILON;

	if (dd1 < epsSqr || dd2 < epsSqr)
	{
		// Handle all degeneracies
		if (dd1 >= epsSqr)
		{
			// Segment 2 is degenerate
			result.fraction1 = Clamp(-rd1 / dd1, 0.0f, 1.0f);
			result.fraction2 = 0.0f;
		}
		else if (dd2 >= epsSqr)
		{
			// Segment 1 is degenerate
			result.fraction1 = 0.0f;
			result.fraction2 = Clamp(rd2 / dd2, 0.0f, 1.0f);
		}
		else
		{
			result.fraction1 = 0.0f;
			result.fraction2 = 0.0f;
		}
	}
	else
	{
		// Non-degenerate segments
		float d12 = DotV(d1, d2);

		float denom = dd1 * dd2 - d12 * d12;

		// Fraction on segment 1
		float f1 = 0.0f;
		if (denom != 0.0f)
		{
			// not parallel
			f1 = Clamp((d12 * rd2 - rd1 * dd2) / denom, 0.0f, 1.0f);
		}

		// Compute point on segment 2 closest to p1 + f1 * d1
		float f2 = (d12 * f1 + rd2) / dd2;

		// Clamping of segment 2 requires a do over on segment 1
		if (f2 < 0.0f)
		{
			f2 = 0.0f;
			f1 = Clamp(-rd1 / dd1, 0.0f, 1.0f);
		}
		else if (f2 > 1.0f)
		{
			f2 = 1.0f;
			f1 = Clamp((d12 - rd1) / dd1, 0.0f, 1.0f);
		}

		result.fraction1 = f1;
		result.fraction2 = f2;
	}

	result.closest1 = MulAddV(p1, result.fraction1, d1);
	result.closest2 = MulAddV(p2, result.fraction2, d2);
	result.distanceSquared = DistSqV(result.closest1, result.closest2);
	return result;
}

// GJK using Voronoi regions (Christer Ericson) and Barycentric coordinates.

gjk::DistanceProxy
ie::gjk::MakeProxy(const vec2* vertices, int32_t count, float radius)
{
	count = Min(count, MAX_POLY_VERTS);
	DistanceProxy proxy;
	for (int32_t i = 0; i < count; ++i) { proxy.vertices[i] = vertices[i]; }
	proxy.count = count;
	proxy.radius = radius;
	return proxy;
}

static ie::vec2
Weight2(float a1, vec2 w1, float a2, vec2 w2)
{
	return {a1 * w1.x + a2 * w2.x, a1 * w1.y + a2 * w2.y};
}

static vec2
Weight3(float a1, vec2 w1, float a2, vec2 w2, float a3, vec2 w3)
{
	return {a1 * w1.x + a2 * w2.x + a3 * w3.x, a1 * w1.y + a2 * w2.y + a3 * w3.y};
}

static int32_t
FindSupport(const gjk::DistanceProxy* proxy, vec2 direction)
{
	int32_t bestIndex = 0;
	float bestValue = DotV(proxy->vertices[0], direction);
	for (int32_t i = 1; i < proxy->count; ++i)
	{
		float value = DotV(proxy->vertices[i], direction);
		if (value > bestValue)
		{
			bestIndex = i;
			bestValue = value;
		}
	}

	return bestIndex;
}

struct SimplexVertex
{
	vec2 wA; // support point in proxyA
	vec2 wB; // support point in proxyB
	vec2 w; // wB - wA
	float a; // barycentric coordinate for closest point
	int32_t indexA; // wA index
	int32_t indexB; // wB index
};

class Simplex
{
public:
	SimplexVertex v1_, v2_, v3_;
	int32_t count_;

	// Make simplex from cache.
	Simplex(
	    const gjk::DistanceCache* cache,
	    const gjk::DistanceProxy* proxyA,
	    gjk::Xf2d transformA,
	    const gjk::DistanceProxy* proxyB,
	    gjk::Xf2d transformB);

	Simplex() = default;

	float Metric() const;
	vec2 Closest() const;
	vec2 ComputeSearchDirection() const;
	void ComputeWitnessPoints(vec2* a, vec2* b) const;
	void Solve2();
	void Solve3();
};

float
Simplex::Metric() const
{
	switch (count_)
	{
		case 0: assert(0); return 0.0f;
		case 1: return 0.0f;
		case 2: return DistV(v1_.w, v2_.w);
		case 3: return CrossV(SubV(v2_.w, v1_.w), SubV(v3_.w, v1_.w));
		default: assert(0); return 0.0f;
	}
}

vec2
Simplex::Closest() const
{
	switch (count_)
	{
		case 0: assert(0); return {0.f, 0.f};
		case 1: return v1_.w;
		case 2: return Weight2(v1_.a, v1_.w, v2_.a, v2_.w);
		case 3: return {0.f, 0.f};
		default: assert(0); return {0.f, 0.f};
	}
}

Simplex::Simplex(
    const gjk::DistanceCache* cache,
    const gjk::DistanceProxy* proxyA,
    gjk::Xf2d transformA,
    const gjk::DistanceProxy* proxyB,
    gjk::Xf2d transformB)
{
	assert(cache->count <= 3);

	// Copy data from cache.
	count_ = cache->count;

	SimplexVertex* vertices[] = {&v1_, &v2_, &v3_};
	for (int32_t i = 0; i < count_; ++i)
	{
		SimplexVertex* v = vertices[i];
		v->indexA = cache->indexA[i];
		v->indexB = cache->indexB[i];
		vec2 wALocal = proxyA->vertices[v->indexA];
		vec2 wBLocal = proxyB->vertices[v->indexB];
		v->wA = TransformPoint(transformA, wALocal);
		v->wB = TransformPoint(transformB, wBLocal);
		v->w = SubV(v->wB, v->wA);

		// invalid.
		v->a = -1.0f;
	}

	// If the cache is empty or invalid ...
	if (count_ == 0)
	{
		SimplexVertex* v = vertices[0];
		v->indexA = 0;
		v->indexB = 0;
		vec2 wALocal = proxyA->vertices[0];
		vec2 wBLocal = proxyB->vertices[0];
		v->wA = TransformPoint(transformA, wALocal);
		v->wB = TransformPoint(transformB, wBLocal);
		v->w = SubV(v->wB, v->wA);
		v->a = 1.0f;
		count_ = 1;
	}
}

static void
MakeSimplexCache(gjk::DistanceCache* cache, const Simplex* simplex)
{
	cache->metric = simplex->Metric();
	cache->count = (uint16_t)simplex->count_;
	const SimplexVertex* vertices[] = {&simplex->v1_, &simplex->v2_, &simplex->v3_};
	for (int32_t i = 0; i < simplex->count_; ++i)
	{
		cache->indexA[i] = (uint8_t)vertices[i]->indexA;
		cache->indexB[i] = (uint8_t)vertices[i]->indexB;
	}
}

vec2
Simplex::ComputeSearchDirection() const
{
	switch (count_)
	{
		case 1: return NegV(v1_.w);

		case 2: {
			vec2 e12 = SubV(v2_.w, v1_.w);
			float sgn = CrossV(e12, NegV(v1_.w));
			if (sgn > 0.0f)
			{
				// Origin is left of e12.
				return CrossSV(1.0f, e12);
			}
			else
			{
				// Origin is right of e12.
				return CrossVS(e12, 1.0f);
			}
		}

		default: assert(0); return {0.f, 0.f};
	}
}

void
Simplex::ComputeWitnessPoints(vec2* a, vec2* b) const
{
	switch (count_)
	{
		case 0: assert(0); break;

		case 1:
			*a = v1_.wA;
			*b = v1_.wB;
			break;

		case 2:
			*a = Weight2(v1_.a, v1_.wA, v2_.a, v2_.wA);
			*b = Weight2(v1_.a, v1_.wB, v2_.a, v2_.wB);
			break;

		case 3:
			*a = Weight3(v1_.a, v1_.wA, v2_.a, v2_.wA, v3_.a, v3_.wA);
			// TODO why are these not equal?
			//*b = Weight3(v1.a, v1.wB, v2.a, v2.wB, v3.a, v3.wB);
			*b = *a;
			break;

		default: assert(0); break;
	}
}

void
Simplex::Solve2()
{
	// Solve a line segment using barycentric coordinates.
	//
	// p = a1 * w1 + a2 * w2
	// a1 + a2 = 1
	//
	// The vector from the origin to the closest point on the line is
	// perpendicular to the line.
	// e12 = w2 - w1
	// dot(p, e) = 0
	// a1 * dot(w1, e) + a2 * dot(w2, e) = 0
	//
	// 2-by-2 linear system
	// [1      1     ][a1] = [1]
	// [w1.e12 w2.e12][a2] = [0]
	//
	// Define
	// d12_1 =  dot(w2, e12)
	// d12_2 = -dot(w1, e12)
	// d12 = d12_1 + d12_2
	//
	// Solution
	// a1 = d12_1 / d12
	// a2 = d12_2 / d12

	vec2 w1 = v1_.w;
	vec2 w2 = v2_.w;
	vec2 e12 = SubV(w2, w1);

	// w1 region
	float d12_2 = -DotV(w1, e12);
	if (d12_2 <= 0.0f)
	{
		// a2 <= 0, so we clamp it to 0
		v1_.a = 1.0f;
		count_ = 1;
		return;
	}

	// w2 region
	float d12_1 = DotV(w2, e12);
	if (d12_1 <= 0.0f)
	{
		// a1 <= 0, so we clamp it to 0
		v2_.a = 1.0f;
		count_ = 1;
		v1_ = v2_;
		return;
	}

	// Must be in e12 region.
	float inv_d12 = 1.0f / (d12_1 + d12_2);
	v1_.a = d12_1 * inv_d12;
	v2_.a = d12_2 * inv_d12;
	count_ = 2;
}

void
Simplex::Solve3()
{
	vec2 w1 = v1_.w;
	vec2 w2 = v2_.w;
	vec2 w3 = v3_.w;

	// Edge12
	// [1      1     ][a1] = [1]
	// [w1.e12 w2.e12][a2] = [0]
	// a3 = 0
	vec2 e12 = SubV(w2, w1);
	float w1e12 = DotV(w1, e12);
	float w2e12 = DotV(w2, e12);
	float d12_1 = w2e12;
	float d12_2 = -w1e12;

	// Edge13
	// [1      1     ][a1] = [1]
	// [w1.e13 w3.e13][a3] = [0]
	// a2 = 0
	vec2 e13 = SubV(w3, w1);
	float w1e13 = DotV(w1, e13);
	float w3e13 = DotV(w3, e13);
	float d13_1 = w3e13;
	float d13_2 = -w1e13;

	// Edge23
	// [1      1     ][a2] = [1]
	// [w2.e23 w3.e23][a3] = [0]
	// a1 = 0
	vec2 e23 = SubV(w3, w2);
	float w2e23 = DotV(w2, e23);
	float w3e23 = DotV(w3, e23);
	float d23_1 = w3e23;
	float d23_2 = -w2e23;

	// Triangle123
	float n123 = CrossV(e12, e13);

	float d123_1 = n123 * CrossV(w2, w3);
	float d123_2 = n123 * CrossV(w3, w1);
	float d123_3 = n123 * CrossV(w1, w2);

	// w1 region
	if (d12_2 <= 0.0f && d13_2 <= 0.0f)
	{
		v1_.a = 1.0f;
		count_ = 1;
		return;
	}

	// e12
	if (d12_1 > 0.0f && d12_2 > 0.0f && d123_3 <= 0.0f)
	{
		float inv_d12 = 1.0f / (d12_1 + d12_2);
		v1_.a = d12_1 * inv_d12;
		v2_.a = d12_2 * inv_d12;
		count_ = 2;
		return;
	}

	// e13
	if (d13_1 > 0.0f && d13_2 > 0.0f && d123_2 <= 0.0f)
	{
		float inv_d13 = 1.0f / (d13_1 + d13_2);
		v1_.a = d13_1 * inv_d13;
		v3_.a = d13_2 * inv_d13;
		count_ = 2;
		v2_ = v3_;
		return;
	}

	// w2 region
	if (d12_1 <= 0.0f && d23_2 <= 0.0f)
	{
		v2_.a = 1.0f;
		count_ = 1;
		v1_ = v2_;
		return;
	}

	// w3 region
	if (d13_1 <= 0.0f && d23_1 <= 0.0f)
	{
		v3_.a = 1.0f;
		count_ = 1;
		v1_ = v3_;
		return;
	}

	// e23
	if (d23_1 > 0.0f && d23_2 > 0.0f && d123_1 <= 0.0f)
	{
		float inv_d23 = 1.0f / (d23_1 + d23_2);
		v2_.a = d23_1 * inv_d23;
		v3_.a = d23_2 * inv_d23;
		count_ = 2;
		v1_ = v3_;
		return;
	}

	// Must be in triangle123
	float inv_d123 = 1.0f / (d123_1 + d123_2 + d123_3);
	v1_.a = d123_1 * inv_d123;
	v2_.a = d123_2 * inv_d123;
	v3_.a = d123_3 * inv_d123;
	count_ = 3;
}

gjk::DistanceOutput
ie::gjk::ShapeDistance(DistanceCache* RESTRICT cache, const DistanceInput* RESTRICT input)
{
	DistanceOutput output{};

	const DistanceProxy* proxyA = &input->proxyA;
	const DistanceProxy* proxyB = &input->proxyB;

	Xf2d transformA = input->transformA;
	Xf2d transformB = input->transformB;

	// Initialize the simplex.
	Simplex simplex{cache, proxyA, transformA, proxyB, transformB};

	// Get simplex vertices as an array.
	SimplexVertex* vertices[] = {&simplex.v1_, &simplex.v2_, &simplex.v3_};
	const int32_t k_maxIters = 20;

	// These store the vertices of the last simplex so that we
	// can check for duplicates and prevent cycling.
	int32_t saveA[3], saveB[3];
	int32_t saveCount = 0;

	// Main iteration loop.
	int32_t iter = 0;
	while (iter < k_maxIters)
	{
		// Copy simplex so we can identify duplicates.
		saveCount = simplex.count_;
		for (int32_t i = 0; i < saveCount; ++i)
		{
			saveA[i] = vertices[i]->indexA;
			saveB[i] = vertices[i]->indexB;
		}

		switch (simplex.count_)
		{
			case 1: break;

			case 2: simplex.Solve2(); break;

			case 3: simplex.Solve3(); break;

			default: assert(0);
		}

		// If we have 3 points, then the origin is in the corresponding triangle.
		if (simplex.count_ == 3) { break; }

		// Get search direction.
		vec2 d = simplex.ComputeSearchDirection();

		// Ensure the search direction is numerically fit.
		if (DotV(d, d) < FLT_EPSILON * FLT_EPSILON)
		{
			// The origin is probably contained by a line segment
			// or triangle. Thus the shapes are overlapped.

			// We can't return zero here even though there may be overlap.
			// In case the simplex is a point, segment, or triangle it is difficult
			// to determine if the origin is contained in the CSO or very close to it.
			break;
		}

		// Compute a tentative new simplex vertex using support points.
		SimplexVertex* vertex = vertices[simplex.count_];
		vertex->indexA = FindSupport(proxyA, InvRotateVector(transformA.q, NegV(d)));
		vertex->wA = TransformPoint(transformA, proxyA->vertices[vertex->indexA]);
		vertex->indexB = FindSupport(proxyB, InvRotateVector(transformB.q, d));
		vertex->wB = TransformPoint(transformB, proxyB->vertices[vertex->indexB]);
		vertex->w = SubV(vertex->wB, vertex->wA);

		// Iteration count is equated to the number of support point calls.
		++iter;

		// Check for duplicate support points. This is the main termination criteria.
		bool duplicate = false;
		for (int32_t i = 0; i < saveCount; ++i)
		{
			if (vertex->indexA == saveA[i] && vertex->indexB == saveB[i])
			{
				duplicate = true;
				break;
			}
		}

		// If we found a duplicate support point we must exit to avoid cycling.
		if (duplicate) { break; }

		// New vertex is ok and needed.
		++simplex.count_;
	}

	// Prepare output
	simplex.ComputeWitnessPoints(&output.pointA, &output.pointB);
	output.distance = DistV(output.pointA, output.pointB);
	output.iterations = iter;

	// Cache the simplex
	MakeSimplexCache(cache, &simplex);

	// Apply radii if requested
	if (input->useRadii)
	{
		if (output.distance < FLT_EPSILON)
		{
			// Shapes are too close to safely compute normal
			vec2 p = {
			    0.5f * (output.pointA.x + output.pointB.x),
			    0.5f * (output.pointA.y + output.pointB.y)};
			output.pointA = p;
			output.pointB = p;
			output.distance = 0.0f;
		}
		else
		{
			// Keep closest points on perimeter even if overlapped, this way
			// the points move smoothly.
			float rA = proxyA->radius;
			float rB = proxyB->radius;
			output.distance = Max(0.0f, output.distance - rA - rB);
			vec2 normal = NormV(SubV(output.pointB, output.pointA));
			vec2 offsetA = {rA * normal.x, rA * normal.y};
			vec2 offsetB = {rB * normal.x, rB * normal.y};
			output.pointA = AddV(output.pointA, offsetA);
			output.pointB = SubV(output.pointB, offsetB);
		}
	}

	return output;
}

// GJK-raycast
// Algorithm by Gino van den Bergen.
// "Smooth Mesh Contacts with GJK" in Game Physics Pearls. 2010
// TODO this is failing when used to raycast a box
gjk::RayHit
ie::gjk::ShapeCast(const ShapeCastInput* input)
{
	RayHit output{};

	const DistanceProxy* proxyA = &input->proxyA;
	const DistanceProxy* proxyB = &input->proxyB;

	float radius = proxyA->radius + proxyB->radius;

	Xf2d xfA = input->transformA;
	Xf2d xfB = input->transformB;

	vec2 r = input->translationB;
	vec2 n = {0.f, 0.f};
	float lambda = 0.0f;
	float maxFraction = input->maxFraction;

	// Initial simplex
	Simplex simplex;
	simplex.count_ = 0;

	// Get simplex vertices as an array.
	SimplexVertex* vertices[] = {&simplex.v1_, &simplex.v2_, &simplex.v3_};

	// Get support point in -r direction
	int32_t indexA = FindSupport(proxyA, InvRotateVector(xfA.q, NegV(r)));
	vec2 wA = TransformPoint(xfA, proxyA->vertices[indexA]);
	int32_t indexB = FindSupport(proxyB, InvRotateVector(xfB.q, r));
	vec2 wB = TransformPoint(xfB, proxyB->vertices[indexB]);
	vec2 v = SubV(wA, wB);

	// Sigma is the target distance between proxies
	const float sigma = Max(LINEAR_SLOP, radius - LINEAR_SLOP);

	// Main iteration loop.
	const int32_t k_maxIters = 20;
	int32_t iter = 0;
	while (iter < k_maxIters && LenV(v) > sigma)
	{
		assert(simplex.count_ < 3);

		output.iterations += 1;

		// Support in direction -v (A - B)
		indexA = FindSupport(proxyA, InvRotateVector(xfA.q, NegV(v)));
		wA = TransformPoint(xfA, proxyA->vertices[indexA]);
		indexB = FindSupport(proxyB, InvRotateVector(xfB.q, v));
		wB = TransformPoint(xfB, proxyB->vertices[indexB]);
		vec2 p = SubV(wA, wB);

		// -v is a normal at p
		v = NormV(v);

		// Intersect ray with plane
		float vp = DotV(v, p);
		float vr = DotV(v, r);
		if (vp - sigma > lambda * vr)
		{
			if (vr <= 0.0f) { return output; }

			lambda = (vp - sigma) / vr;
			if (lambda > maxFraction) { return output; }

			n = {-v.x, -v.y};
			simplex.count_ = 0;
		}

		// Reverse simplex since it works with B - A.
		// Shift by lambda * r because we want the closest point to the current clip point.
		// Note that the support point p is not shifted because we want the plane equation
		// to be formed in unshifted space.
		SimplexVertex* vertex = vertices[simplex.count_];
		vertex->indexA = indexB;
		vertex->wA = {wB.x + lambda * r.x, wB.y + lambda * r.y};
		vertex->indexB = indexA;
		vertex->wB = wA;
		vertex->w = SubV(vertex->wB, vertex->wA);
		vertex->a = 1.0f;
		simplex.count_ += 1;

		switch (simplex.count_)
		{
			case 1: break;

			case 2: simplex.Solve2(); break;

			case 3: simplex.Solve3(); break;

			default: assert(0);
		}

		// If we have 3 points, then the origin is in the corresponding triangle.
		if (simplex.count_ == 3)
		{
			// Overlap
			return output;
		}

		// Get search direction.
		v = simplex.Closest();

		// Iteration count is equated to the number of support point calls.
		++iter;
	}

	if (iter == 0)
	{
		// Initial overlap
		return output;
	}

	// Prepare output.
	vec2 pointA, pointB;
	simplex.ComputeWitnessPoints(&pointB, &pointA);

	if (DotV(v, v) > 0.0f) { n = NormV(NegV(v)); }

	float radiusA = proxyA->radius;
	output.point = {pointA.x + radiusA * n.x, pointA.y + radiusA * n.y};
	output.normal = n;
	output.fraction = lambda;
	output.iterations = iter;
	output.hit = true;
	return output;
}

enum class SepType
{
	kPoint,
	kFaceA,
	kFaceB
};

struct SepFunc
{
	const gjk::DistanceProxy* proxyA;
	const gjk::DistanceProxy* proxyB;
	gjk::Sweep sweepA, sweepB;
	vec2 localPoint;
	vec2 axis;
	SepType type;

	static SepFunc Make(
	    const gjk::DistanceCache* cache,
	    const gjk::DistanceProxy* proxyA,
	    const gjk::Sweep* sweepA,
	    const gjk::DistanceProxy* proxyB,
	    const gjk::Sweep* sweepB,
	    float t1)
	{
		SepFunc f;

		f.proxyA = proxyA;
		f.proxyB = proxyB;
		int32_t count = cache->count;
		assert(0 < count && count < 3);

		f.sweepA = *sweepA;
		f.sweepB = *sweepB;

		gjk::Xf2d xfA = GetSweepTransform(sweepA, t1);
		gjk::Xf2d xfB = GetSweepTransform(sweepB, t1);

		if (count == 1)
		{
			f.type = SepType::kPoint;
			vec2 localPointA = proxyA->vertices[cache->indexA[0]];
			vec2 localPointB = proxyB->vertices[cache->indexB[0]];
			vec2 pointA = TransformPoint(xfA, localPointA);
			vec2 pointB = TransformPoint(xfB, localPointB);
			f.axis = NormV(SubV(pointB, pointA));
			f.localPoint = {0.f, 0.f};
			return f;
		}

		if (cache->indexA[0] == cache->indexA[1])
		{
			// Two points on B and one on A.
			f.type = SepType::kFaceB;
			vec2 localPointB1 = proxyB->vertices[cache->indexB[0]];
			vec2 localPointB2 = proxyB->vertices[cache->indexB[1]];

			f.axis = CrossVS(SubV(localPointB2, localPointB1), 1.0f);
			f.axis = NormV(f.axis);
			vec2 normal = RotateVector(xfB.q, f.axis);

			f.localPoint = {
			    0.5f * (localPointB1.x + localPointB2.x),
			    0.5f * (localPointB1.y + localPointB2.y)};
			vec2 pointB = TransformPoint(xfB, f.localPoint);

			vec2 localPointA = proxyA->vertices[cache->indexA[0]];
			vec2 pointA = TransformPoint(xfA, localPointA);

			float s = DotV(SubV(pointA, pointB), normal);
			if (s < 0.0f) { f.axis = NegV(f.axis); }
			return f;
		}

		// Two points on A and one or two points on B.
		f.type = SepType::kFaceA;
		vec2 localPointA1 = proxyA->vertices[cache->indexA[0]];
		vec2 localPointA2 = proxyA->vertices[cache->indexA[1]];

		f.axis = CrossVS(SubV(localPointA2, localPointA1), 1.0f);
		f.axis = NormV(f.axis);
		vec2 normal = RotateVector(xfA.q, f.axis);

		f.localPoint = {
		    0.5f * (localPointA1.x + localPointA2.x),
		    0.5f * (localPointA1.y + localPointA2.y)};
		vec2 pointA = TransformPoint(xfA, f.localPoint);

		vec2 localPointB = proxyB->vertices[cache->indexB[0]];
		vec2 pointB = TransformPoint(xfB, localPointB);

		float s = DotV(SubV(pointB, pointA), normal);
		if (s < 0.0f) { f.axis = NegV(f.axis); }
		return f;
	}
};

float
FindMinSeparation(const SepFunc* f, int32_t* indexA, int32_t* indexB, float t)
{
	gjk::Xf2d xfA = GetSweepTransform(&f->sweepA, t);
	gjk::Xf2d xfB = GetSweepTransform(&f->sweepB, t);

	switch (f->type)
	{
		case SepType::kPoint: {
			vec2 axisA = InvRotateVector(xfA.q, f->axis);
			vec2 axisB = InvRotateVector(xfB.q, NegV(f->axis));

			*indexA = FindSupport(f->proxyA, axisA);
			*indexB = FindSupport(f->proxyB, axisB);

			vec2 localPointA = f->proxyA->vertices[*indexA];
			vec2 localPointB = f->proxyB->vertices[*indexB];

			vec2 pointA = TransformPoint(xfA, localPointA);
			vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = DotV(SubV(pointB, pointA), f->axis);
			return separation;
		}

		case SepType::kFaceA: {
			vec2 normal = RotateVector(xfA.q, f->axis);
			vec2 pointA = TransformPoint(xfA, f->localPoint);

			vec2 axisB = InvRotateVector(xfB.q, NegV(normal));

			*indexA = -1;
			*indexB = FindSupport(f->proxyB, axisB);

			vec2 localPointB = f->proxyB->vertices[*indexB];
			vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = DotV(SubV(pointB, pointA), normal);
			return separation;
		}

		case SepType::kFaceB: {
			vec2 normal = RotateVector(xfB.q, f->axis);
			vec2 pointB = TransformPoint(xfB, f->localPoint);

			vec2 axisA = InvRotateVector(xfA.q, NegV(normal));

			*indexB = -1;
			*indexA = FindSupport(f->proxyA, axisA);

			vec2 localPointA = f->proxyA->vertices[*indexA];
			vec2 pointA = TransformPoint(xfA, localPointA);

			float separation = DotV(SubV(pointA, pointB), normal);
			return separation;
		}

		default:
			assert(0);
			*indexA = -1;
			*indexB = -1;
			return 0.0f;
	}
}

float
EvaluateSeparation(const SepFunc* f, int32_t indexA, int32_t indexB, float t)
{
	gjk::Xf2d xfA = GetSweepTransform(&f->sweepA, t);
	gjk::Xf2d xfB = GetSweepTransform(&f->sweepB, t);

	switch (f->type)
	{
		case SepType::kPoint: {
			vec2 localPointA = f->proxyA->vertices[indexA];
			vec2 localPointB = f->proxyB->vertices[indexB];

			vec2 pointA = TransformPoint(xfA, localPointA);
			vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = DotV(SubV(pointB, pointA), f->axis);
			return separation;
		}

		case SepType::kFaceA: {
			vec2 normal = RotateVector(xfA.q, f->axis);
			vec2 pointA = TransformPoint(xfA, f->localPoint);

			vec2 localPointB = f->proxyB->vertices[indexB];
			vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = DotV(SubV(pointB, pointA), normal);
			return separation;
		}

		case SepType::kFaceB: {
			vec2 normal = RotateVector(xfB.q, f->axis);
			vec2 pointB = TransformPoint(xfB, f->localPoint);

			vec2 localPointA = f->proxyA->vertices[indexA];
			vec2 pointA = TransformPoint(xfA, localPointA);

			float separation = DotV(SubV(pointA, pointB), normal);
			return separation;
		}

		default: assert(0); return 0.0f;
	}
}

// CCD via the local separating axis method. This seeks progression
// by computing the largest time at which separation is maintained.
gjk::TOIOutput
ie::gjk::TimeOfImpact(const TOIInput* input)
{
	TOIOutput output;
	output.state = TOIState::kUnknown;
	output.t = input->tMax;

	const DistanceProxy* proxyA = &input->proxyA;
	const DistanceProxy* proxyB = &input->proxyB;

	Sweep sweepA = input->sweepA;
	Sweep sweepB = input->sweepB;

	// Large rotations can make the root finder fail, so normalize the sweep angles.
	float twoPi = 2.0f * PI;
	{
		float d = twoPi * floorf(sweepA.a1 / twoPi);
		sweepA.a1 -= d;
		sweepA.a2 -= d;
	}
	{
		float d = twoPi * floorf(sweepB.a1 / twoPi);
		sweepB.a1 -= d;
		sweepB.a2 -= d;
	}

	float tMax = input->tMax;

	float totalRadius = proxyA->radius + proxyB->radius;
	float target = Max(LINEAR_SLOP, totalRadius + LINEAR_SLOP);
	float tolerance = 0.25f * LINEAR_SLOP;
	assert(target > tolerance);

	float t1 = 0.0f;
	const int32_t k_maxIterations = 20;
	int32_t iter = 0;

	// Prepare input for distance query.
	DistanceCache cache;
	DistanceInput distanceInput;
	distanceInput.proxyA = input->proxyA;
	distanceInput.proxyB = input->proxyB;
	distanceInput.useRadii = false;

	// The outer loop progressively attempts to compute new separating axes.
	// This loop terminates when an axis is repeated (no progress is made).
	for (;;)
	{
		Xf2d xfA = GetSweepTransform(&sweepA, t1);
		Xf2d xfB = GetSweepTransform(&sweepB, t1);

		// Get the distance between shapes. We can also use the results
		// to get a separating axis.
		distanceInput.transformA = xfA;
		distanceInput.transformB = xfB;
		DistanceOutput distanceOutput = ShapeDistance(&cache, &distanceInput);

		// If the shapes are overlapped, we give up on continuous collision.
		if (distanceOutput.distance <= 0.0f)
		{
			// Failure!
			output.state = TOIState::kOverlapped;
			output.t = 0.0f;
			break;
		}

		if (distanceOutput.distance < target + tolerance)
		{
			// Victory!
			output.state = TOIState::kHit;
			output.t = t1;
			break;
		}

		// Initialize the separating axis.
		SepFunc fcn = SepFunc::Make(&cache, proxyA, &sweepA, proxyB, &sweepB, t1);

		// Compute the TOI on the separating axis. We do this by successively
		// resolving the deepest point. This loop is bounded by the number of vertices.
		bool done = false;
		float t2 = tMax;
		int32_t pushBackIter = 0;
		for (;;)
		{
			// Find the deepest point at t2. Store the witness point indices.
			int32_t indexA, indexB;
			float s2 = FindMinSeparation(&fcn, &indexA, &indexB, t2);

			// Is the final configuration separated?
			if (s2 > target + tolerance)
			{
				// Victory!
				output.state = TOIState::kSeparated;
				output.t = tMax;
				done = true;
				break;
			}

			// Has the separation reached tolerance?
			if (s2 > target - tolerance)
			{
				// Advance the sweeps
				t1 = t2;
				break;
			}

			// Compute the initial separation of the witness points.
			float s1 = EvaluateSeparation(&fcn, indexA, indexB, t1);

			// Check for initial overlap. This might happen if the root finder
			// runs out of iterations.
			if (s1 < target - tolerance)
			{
				output.state = TOIState::kFailed;
				output.t = t1;
				done = true;
				break;
			}

			// Check for touching
			if (s1 <= target + tolerance)
			{
				// Victory! t1 should hold the TOI (could be 0.0).
				output.state = TOIState::kHit;
				output.t = t1;
				done = true;
				break;
			}

			// Compute 1D root of: f(x) - target = 0
			int32_t rootIterCount = 0;
			float a1 = t1, a2 = t2;
			for (;;)
			{
				// Use a mix of the secant rule and bisection.
				float t;
				if (rootIterCount & 1)
				{
					// Secant rule to improve convergence.
					t = a1 + (target - s1) * (a2 - a1) / (s2 - s1);
				}
				else
				{
					// Bisection to guarantee progress.
					t = 0.5f * (a1 + a2);
				}

				++rootIterCount;

				float s = EvaluateSeparation(&fcn, indexA, indexB, t);

				if (Abs(s - target) < tolerance)
				{
					// t2 holds a tentative value for t1
					t2 = t;
					break;
				}

				// Ensure we continue to bracket the root.
				if (s > target)
				{
					a1 = t;
					s1 = s;
				}
				else
				{
					a2 = t;
					s2 = s;
				}

				if (rootIterCount == 50) { break; }
			}

			++pushBackIter;

			if (pushBackIter == MAX_POLY_VERTS) { break; }
		}

		++iter;

		if (done) { break; }

		if (iter == k_maxIterations)
		{
			// Root finder got stuck. Semi-victory.
			output.state = TOIState::kFailed;
			output.t = t1;
			break;
		}
	}

	return output;
}

bool
ie::gjk::PointInCircle(vec2 point, const Circle* shape)
{
	vec2 center = shape->point;
	return DistSqV(point, center) <= shape->radius * shape->radius;
}

bool
ie::gjk::PointInCapsule(vec2 point, const Capsule* shape)
{
	float rr = shape->radius * shape->radius;
	vec2 p1 = shape->point1;
	vec2 p2 = shape->point2;

	vec2 d = SubV(p2, p1);
	float dd = DotV(d, d);
	if (dd == 0.0f)
	{
		// Capsule is really a circle
		return DistSqV(point, p1) <= rr;
	}

	// Get closest point on capsule segment
	// c = p1 + t * d
	// dot(point - c, d) = 0
	// dot(point - p1 - t * d, d) = 0
	// t = dot(point - p1, d) / dot(d, d)
	float t = DotV(SubV(point, p1), d) / dd;
	t = Clamp(t, 0.0f, 1.0f);
	vec2 c = MulAddV(p1, t, d);

	// Is query point within radius around closest point?
	return DistSqV(point, c) <= rr;
}

bool
ie::gjk::PointInPolygon(vec2 point, const Polygon* shape)
{
	DistanceInput input{};
	input.proxyA = MakeProxy(shape->vertices, shape->count, 0.0f);
	input.proxyB = MakeProxy(&point, 1, 0.0f);
	input.transformA.SetIdentity();
	input.transformB.SetIdentity();
	input.useRadii = false;

	DistanceCache cache{};
	DistanceOutput output = ShapeDistance(&cache, &input);

	return output.distance <= shape->radius;
}

AABB
ie::gjk::ComputeCircleAABB(const Circle* shape, const Xf2d& xf)
{
	vec2 p = TransformPoint(xf, shape->point);
	float r = shape->radius;

	AABB aabb = {{p.x - r, p.y - r}, {p.x + r, p.y + r}};
	return aabb;
}

AABB
ie::gjk::ComputeCapsuleAABB(const Capsule* shape, const Xf2d& xf)
{
	vec2 v1 = TransformPoint(xf, shape->point1);
	vec2 v2 = TransformPoint(xf, shape->point2);

	vec2 r = {shape->radius, shape->radius};
	vec2 lower = SubV(MinV(v1, v2), r);
	vec2 upper = AddV(MaxV(v1, v2), r);

	AABB aabb = {lower, upper};
	return aabb;
}

AABB
ie::gjk::ComputePolygonAABB(const Polygon* shape, const Xf2d& xf)
{
	assert(shape->count > 0);
	vec2 lower = TransformPoint(xf, shape->vertices[0]);
	vec2 upper = lower;

	for (int32_t i = 1; i < shape->count; ++i)
	{
		vec2 v = TransformPoint(xf, shape->vertices[i]);
		lower = MinV(lower, v);
		upper = MaxV(upper, v);
	}

	vec2 r = {shape->radius, shape->radius};
	lower = SubV(lower, r);
	upper = AddV(upper, r);

	AABB aabb = {lower, upper};
	return aabb;
}

AABB
ie::gjk::ComputeSegmentAABB(const Segment* shape, const Xf2d& xf)
{
	vec2 v1 = TransformPoint(xf, shape->p1);
	vec2 v2 = TransformPoint(xf, shape->p2);

	vec2 lower = MinV(v1, v2);
	vec2 upper = MaxV(v1, v2);

	AABB aabb = {lower, upper};
	return aabb;
}

// quickhull recursion
static gjk::Hull
RecurseHull(vec2 p1, vec2 p2, vec2* ps, int32_t count)
{
	gjk::Hull hull;
	hull.count = 0;

	if (count == 0) { return hull; }

	// create an edge vector pointing from p1 to p2
	vec2 e = NormV(SubV(p2, p1));

	// discard points left of e and find point furthest to the right of e
	vec2 rightPoints[ie::gjk::MAX_POLY_VERTS];
	int32_t rightCount = 0;

	int32_t bestIndex = 0;
	float bestDistance = CrossV(SubV(ps[bestIndex], p1), e);
	if (bestDistance > 0.0f) { rightPoints[rightCount++] = ps[bestIndex]; }

	for (int32_t i = 1; i < count; ++i)
	{
		float distance = CrossV(SubV(ps[i], p1), e);
		if (distance > bestDistance)
		{
			bestIndex = i;
			bestDistance = distance;
		}

		if (distance > 0.0f) { rightPoints[rightCount++] = ps[i]; }
	}

	if (bestDistance < 2.0f * ie::gjk::LINEAR_SLOP) { return hull; }

	vec2 bestPoint = ps[bestIndex];

	// compute hull to the right of p1-bestPoint
	gjk::Hull hull1 = RecurseHull(p1, bestPoint, rightPoints, rightCount);

	// compute hull to the right of bestPoint-p2
	gjk::Hull hull2 = RecurseHull(bestPoint, p2, rightPoints, rightCount);

	// stitch together hulls
	for (int32_t i = 0; i < hull1.count; ++i) { hull.points[hull.count++] = hull1.points[i]; }

	hull.points[hull.count++] = bestPoint;

	for (int32_t i = 0; i < hull2.count; ++i) { hull.points[hull.count++] = hull2.points[i]; }

	assert(hull.count < ie::gjk::MAX_POLY_VERTS);

	return hull;
}

// quickhull algorithm
// - merges vertices based on LINEAR_SLOP
// - removes collinear points using LINEAR_SLOP
// - returns an empty hull if it fails
gjk::Hull
ie::gjk::ComputeHull(const vec2* points, int32_t count)
{
	Hull hull;
	hull.count = 0;

	if (count < 3 || count > MAX_POLY_VERTS)
	{
		// check your data
		return hull;
	}

	count = Min(count, MAX_POLY_VERTS);

	AABB aabb = {{FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX}};

	// Perform aggressive point welding. First point always remains.
	// Also compute the bounding box for later.
	vec2 ps[MAX_POLY_VERTS];
	int32_t n = 0;
	const float tolSqr = 16.0f * LINEAR_SLOP * LINEAR_SLOP;
	for (int32_t i = 0; i < count; ++i)
	{
		aabb.l = MinV(aabb.l, points[i]);
		aabb.u = MaxV(aabb.u, points[i]);

		vec2 vi = points[i];

		bool unique = true;
		for (int32_t j = 0; j < i; ++j)
		{
			vec2 vj = points[j];

			float distSqr = DistSqV(vi, vj);
			if (distSqr < tolSqr)
			{
				unique = false;
				break;
			}
		}

		if (unique) { ps[n++] = vi; }
	}

	if (n < 3)
	{
		// all points very close together, check your data and check your scale
		return hull;
	}

	// Find an extreme point as the first point on the hull
	vec2 c = aabb.Center();
	int32_t f1 = 0;
	float dsq1 = DistSqV(c, ps[f1]);
	for (int32_t i = 1; i < n; ++i)
	{
		float dsq = DistSqV(c, ps[i]);
		if (dsq > dsq1)
		{
			f1 = i;
			dsq1 = dsq;
		}
	}

	// remove p1 from working set
	vec2 p1 = ps[f1];
	ps[f1] = ps[n - 1];
	n = n - 1;

	int32_t f2 = 0;
	float dsq2 = DistSqV(p1, ps[f2]);
	for (int32_t i = 1; i < n; ++i)
	{
		float dsq = DistSqV(p1, ps[i]);
		if (dsq > dsq2)
		{
			f2 = i;
			dsq2 = dsq;
		}
	}

	// remove p2 from working set
	vec2 p2 = ps[f2];
	ps[f2] = ps[n - 1];
	n = n - 1;

	// split the points into points that are left and right of the line p1-p2.
	vec2 rightPoints[MAX_POLY_VERTS - 2];
	int32_t rightCount = 0;

	vec2 leftPoints[MAX_POLY_VERTS - 2];
	int32_t leftCount = 0;

	vec2 e = NormV(SubV(p2, p1));

	for (int32_t i = 0; i < n; ++i)
	{
		float d = CrossV(SubV(ps[i], p1), e);

		// slop used here to skip points that are very close to the line p1-p2
		if (d >= 2.0f * LINEAR_SLOP) { rightPoints[rightCount++] = ps[i]; }
		else if (d <= -2.0f * LINEAR_SLOP) { leftPoints[leftCount++] = ps[i]; }
	}

	// compute hulls on right and left
	Hull hull1 = RecurseHull(p1, p2, rightPoints, rightCount);
	Hull hull2 = RecurseHull(p2, p1, leftPoints, leftCount);

	if (hull1.count == 0 && hull2.count == 0)
	{
		// all points collinear
		return hull;
	}

	// stitch hulls together, preserving CCW winding order
	hull.points[hull.count++] = p1;

	for (int32_t i = 0; i < hull1.count; ++i) { hull.points[hull.count++] = hull1.points[i]; }

	hull.points[hull.count++] = p2;

	for (int32_t i = 0; i < hull2.count; ++i) { hull.points[hull.count++] = hull2.points[i]; }

	assert(hull.count <= MAX_POLY_VERTS);

	// merge collinear
	bool searching = true;
	while (searching && hull.count > 2)
	{
		searching = false;

		for (int32_t i = 0; i < hull.count; ++i)
		{
			int32_t i1 = i;
			int32_t i2 = (i + 1) % hull.count;
			int32_t i3 = (i + 2) % hull.count;

			vec2 s1 = hull.points[i1];
			vec2 s2 = hull.points[i2];
			vec2 s3 = hull.points[i3];

			// unit edge vector for s1-s3
			vec2 r = NormV(SubV(s3, s1));

			float distance = CrossV(SubV(s2, s1), r);
			if (distance <= 2.0f * LINEAR_SLOP)
			{
				// remove midpoint from hull
				for (int32_t j = i2; j < hull.count - 1; ++j)
				{
					hull.points[j] = hull.points[j + 1];
				}
				hull.count -= 1;

				// continue searching for collinear points
				searching = true;

				break;
			}
		}
	}

	if (hull.count < 3)
	{
		// all points collinear, shouldn't be reached since this was validated above
		hull.count = 0;
	}

	return hull;
}

bool
ie::gjk::ValidateHull(const Hull* hull)
{
	if (hull->count < 3 || MAX_POLY_VERTS < hull->count) { return false; }

	// test that every point is behind every edge
	for (int32_t i = 0; i < hull->count; ++i)
	{
		// create an edge vector
		int32_t i1 = i;
		int32_t i2 = i < hull->count - 1 ? i1 + 1 : 0;
		vec2 p = hull->points[i1];
		vec2 e = NormV(SubV(hull->points[i2], p));

		for (int32_t j = 0; j < hull->count; ++j)
		{
			// skip points that subtend the current edge
			if (j == i1 || j == i2) { continue; }

			float distance = CrossV(SubV(hull->points[j], p), e);
			if (distance >= 0.0f) { return false; }
		}
	}

	// test for collinear points
	for (int32_t i = 0; i < hull->count; ++i)
	{
		int32_t i1 = i;
		int32_t i2 = (i + 1) % hull->count;
		int32_t i3 = (i + 2) % hull->count;

		vec2 p1 = hull->points[i1];
		vec2 p2 = hull->points[i2];
		vec2 p3 = hull->points[i3];

		vec2 e = NormV(SubV(p3, p1));

		float distance = CrossV(SubV(p2, p1), e);
		if (distance <= LINEAR_SLOP)
		{
			// p1-p2-p3 are collinear
			return false;
		}
	}

	return true;
}

gjk::RayCast::RayCast(ie::vec2 start, ie::vec2 end, float maxDist, float r)
    : p1(start), p2(end), maxFraction(maxDist), radius(r)
{
}
