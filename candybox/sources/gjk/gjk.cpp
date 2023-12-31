﻿#include "candybox/GJK.hpp"
#include <candybox/assert.hpp>
#include "candybox/Macros.hpp"

using namespace candybox;

#define RESTRICT

gjk::Xf2d
candybox::gjk::GetSweepTransform(const Sweep* sweep, float time)
{
	// https://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/
	Xf2d xf;
	xf.p = m::addv(m::mulsv(1.0f - time, sweep->c1), m::mulsv(time, sweep->c2));
	float angle = (1.0f - time) * sweep->a1 + time * sweep->a2;
	xf.q.set(angle);

	// Shift to origin
	xf.p = m::subv(xf.p, RotateVector(xf.q, sweep->localCenter));
	return xf;
}

/// Follows Ericson 5.1.9 Closest Points of Two Line Segments
gjk::SegmentDistanceResult
candybox::gjk::SegmentDistance(glm::vec2 p1, glm::vec2 q1, glm::vec2 p2, glm::vec2 q2)
{
	SegmentDistanceResult result{};

	glm::vec2 d1 = m::subv(q1, p1);
	glm::vec2 d2 = m::subv(q2, p2);
	glm::vec2 r = m::subv(p1, p2);
	float dd1 = m::dotv(d1, d1);
	float dd2 = m::dotv(d2, d2);
	float rd2 = m::dotv(r, d2);
	float rd1 = m::dotv(r, d1);

	const float epsSqr = FLT_EPSILON * FLT_EPSILON;

	if (dd1 < epsSqr || dd2 < epsSqr)
	{
		// Handle all degeneracies
		if (dd1 >= epsSqr)
		{
			// Segment 2 is degenerate
			result.fraction1 = m::clamp(-rd1 / dd1, 0.0f, 1.0f);
			result.fraction2 = 0.0f;
		}
		else if (dd2 >= epsSqr)
		{
			// Segment 1 is degenerate
			result.fraction1 = 0.0f;
			result.fraction2 = m::clamp(rd2 / dd2, 0.0f, 1.0f);
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
		float d12 = m::dotv(d1, d2);

		float denom = dd1 * dd2 - d12 * d12;

		// Fraction on segment 1
		float f1 = 0.0f;
		if (denom != 0.0f)
		{
			// not parallel
			f1 = m::clamp((d12 * rd2 - rd1 * dd2) / denom, 0.0f, 1.0f);
		}

		// Compute point on segment 2 closest to p1 + f1 * d1
		float f2 = (d12 * f1 + rd2) / dd2;

		// Clamping of segment 2 requires a do over on segment 1
		if (f2 < 0.0f)
		{
			f2 = 0.0f;
			f1 = m::clamp(-rd1 / dd1, 0.0f, 1.0f);
		}
		else if (f2 > 1.0f)
		{
			f2 = 1.0f;
			f1 = m::clamp((d12 - rd1) / dd1, 0.0f, 1.0f);
		}

		result.fraction1 = f1;
		result.fraction2 = f2;
	}

	result.closest1 = m::muladdv(p1, result.fraction1, d1);
	result.closest2 = m::muladdv(p2, result.fraction2, d2);
	result.distanceSquared = m::distsqv(result.closest1, result.closest2);
	return result;
}

// GJK using Voronoi regions (Christer Ericson) and Barycentric coordinates.

gjk::DistanceProxy
candybox::gjk::MakeProxy(const glm::vec2* vertices, int32_t count, float radius)
{
	count = m::min(count, MAX_POLY_VERTS);
	DistanceProxy proxy;
	for (int32_t i = 0; i < count; ++i) { proxy.vertices[i] = vertices[i]; }
	proxy.count = count;
	proxy.radius = radius;
	return proxy;
}

static glm::vec2
Weight2(float a1, glm::vec2 w1, float a2, glm::vec2 w2)
{
	return {a1 * w1.x + a2 * w2.x, a1 * w1.y + a2 * w2.y};
}

static glm::vec2
Weight3(float a1, glm::vec2 w1, float a2, glm::vec2 w2, float a3, glm::vec2 w3)
{
	return {a1 * w1.x + a2 * w2.x + a3 * w3.x, a1 * w1.y + a2 * w2.y + a3 * w3.y};
}

static int32_t
FindSupport(const gjk::DistanceProxy* proxy, glm::vec2 direction)
{
	int32_t bestIndex = 0;
	float bestValue = m::dotv(proxy->vertices[0], direction);
	for (int32_t i = 1; i < proxy->count; ++i)
	{
		float value = m::dotv(proxy->vertices[i], direction);
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
	glm::vec2 wA; // support point in proxyA
	glm::vec2 wB; // support point in proxyB
	glm::vec2 w; // wB - wA
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
	glm::vec2 Closest() const;
	glm::vec2 ComputeSearchDirection() const;
	void ComputeWitnessPoints(glm::vec2* a, glm::vec2* b) const;
	void Solve2();
	void Solve3();
};

float
Simplex::Metric() const
{
	switch (count_)
	{
		case 0: candybox_assert(0); return 0.0f;
		case 1: return 0.0f;
		case 2: return m::distv(v1_.w, v2_.w);
		case 3: return m::crossv(m::subv(v2_.w, v1_.w), m::subv(v3_.w, v1_.w));
		default: candybox_assert(0); return 0.0f;
	}
}

glm::vec2
Simplex::Closest() const
{
	switch (count_)
	{
		case 0: candybox_assert(0); return {0.f, 0.f};
		case 1: return v1_.w;
		case 2: return Weight2(v1_.a, v1_.w, v2_.a, v2_.w);
		case 3: return {0.f, 0.f};
		default: candybox_assert(0); return {0.f, 0.f};
	}
}

Simplex::Simplex(
    const gjk::DistanceCache* cache,
    const gjk::DistanceProxy* proxyA,
    gjk::Xf2d transformA,
    const gjk::DistanceProxy* proxyB,
    gjk::Xf2d transformB)
{
	candybox_assert(cache->count <= 3);

	// Copy data from cache.
	count_ = cache->count;

	SimplexVertex* vertices[] = {&v1_, &v2_, &v3_};
	for (int32_t i = 0; i < count_; ++i)
	{
		SimplexVertex* v = vertices[i];
		v->indexA = cache->indexA[i];
		v->indexB = cache->indexB[i];
		glm::vec2 wALocal = proxyA->vertices[v->indexA];
		glm::vec2 wBLocal = proxyB->vertices[v->indexB];
		v->wA = TransformPoint(transformA, wALocal);
		v->wB = TransformPoint(transformB, wBLocal);
		v->w = m::subv(v->wB, v->wA);

		// invalid.
		v->a = -1.0f;
	}

	// If the cache is empty or invalid ...
	if (count_ == 0)
	{
		SimplexVertex* v = vertices[0];
		v->indexA = 0;
		v->indexB = 0;
		glm::vec2 wALocal = proxyA->vertices[0];
		glm::vec2 wBLocal = proxyB->vertices[0];
		v->wA = TransformPoint(transformA, wALocal);
		v->wB = TransformPoint(transformB, wBLocal);
		v->w = m::subv(v->wB, v->wA);
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

glm::vec2
Simplex::ComputeSearchDirection() const
{
	switch (count_)
	{
		case 1: return m::negv(v1_.w);

		case 2: {
			glm::vec2 e12 = m::subv(v2_.w, v1_.w);
			float sgn = m::crossv(e12, m::negv(v1_.w));
			if (sgn > 0.0f)
			{
				// Origin is left of e12.
				return m::crosssv(1.0f, e12);
			}
			else
			{
				// Origin is right of e12.
				return m::crossvs(e12, 1.0f);
			}
		}

		default: candybox_assert(0); return {0.f, 0.f};
	}
}

void
Simplex::ComputeWitnessPoints(glm::vec2* a, glm::vec2* b) const
{
	switch (count_)
	{
		case 0: candybox_assert(0); break;

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

		default: candybox_assert(0); break;
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

	glm::vec2 w1 = v1_.w;
	glm::vec2 w2 = v2_.w;
	glm::vec2 e12 = m::subv(w2, w1);

	// w1 region
	float d12_2 = -m::dotv(w1, e12);
	if (d12_2 <= 0.0f)
	{
		// a2 <= 0, so we clamp it to 0
		v1_.a = 1.0f;
		count_ = 1;
		return;
	}

	// w2 region
	float d12_1 = m::dotv(w2, e12);
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
	glm::vec2 w1 = v1_.w;
	glm::vec2 w2 = v2_.w;
	glm::vec2 w3 = v3_.w;

	// Edge12
	// [1      1     ][a1] = [1]
	// [w1.e12 w2.e12][a2] = [0]
	// a3 = 0
	glm::vec2 e12 = m::subv(w2, w1);
	float w1e12 = m::dotv(w1, e12);
	float w2e12 = m::dotv(w2, e12);
	float d12_1 = w2e12;
	float d12_2 = -w1e12;

	// Edge13
	// [1      1     ][a1] = [1]
	// [w1.e13 w3.e13][a3] = [0]
	// a2 = 0
	glm::vec2 e13 = m::subv(w3, w1);
	float w1e13 = m::dotv(w1, e13);
	float w3e13 = m::dotv(w3, e13);
	float d13_1 = w3e13;
	float d13_2 = -w1e13;

	// Edge23
	// [1      1     ][a2] = [1]
	// [w2.e23 w3.e23][a3] = [0]
	// a1 = 0
	glm::vec2 e23 = m::subv(w3, w2);
	float w2e23 = m::dotv(w2, e23);
	float w3e23 = m::dotv(w3, e23);
	float d23_1 = w3e23;
	float d23_2 = -w2e23;

	// Triangle123
	float n123 = m::crossv(e12, e13);

	float d123_1 = n123 * m::crossv(w2, w3);
	float d123_2 = n123 * m::crossv(w3, w1);
	float d123_3 = n123 * m::crossv(w1, w2);

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
candybox::gjk::ShapeDistance(DistanceCache* RESTRICT cache, const DistanceInput* RESTRICT input)
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

			default: candybox_assert(0);
		}

		// If we have 3 points, then the origin is in the corresponding triangle.
		if (simplex.count_ == 3) { break; }

		// Get search direction.
		glm::vec2 d = simplex.ComputeSearchDirection();

		// Ensure the search direction is numerically fit.
		if (m::dotv(d, d) < FLT_EPSILON * FLT_EPSILON)
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
		vertex->indexA = FindSupport(proxyA, InvRotateVector(transformA.q, m::negv(d)));
		vertex->wA = TransformPoint(transformA, proxyA->vertices[vertex->indexA]);
		vertex->indexB = FindSupport(proxyB, InvRotateVector(transformB.q, d));
		vertex->wB = TransformPoint(transformB, proxyB->vertices[vertex->indexB]);
		vertex->w = m::subv(vertex->wB, vertex->wA);

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
	output.distance = m::distv(output.pointA, output.pointB);
	output.iterations = iter;

	// Cache the simplex
	MakeSimplexCache(cache, &simplex);

	// Apply radii if requested
	if (input->useRadii)
	{
		if (output.distance < FLT_EPSILON)
		{
			// Shapes are too close to safely compute normal
			glm::vec2 p = {
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
			output.distance = m::max(0.0f, output.distance - rA - rB);
			glm::vec2 normal = m::normv(m::subv(output.pointB, output.pointA));
			glm::vec2 offsetA = {rA * normal.x, rA * normal.y};
			glm::vec2 offsetB = {rB * normal.x, rB * normal.y};
			output.pointA = m::addv(output.pointA, offsetA);
			output.pointB = m::subv(output.pointB, offsetB);
		}
	}

	return output;
}

// GJK-raycast
// Algorithm by Gino van den Bergen.
// "Smooth Mesh Contacts with GJK" in Game Physics Pearls. 2010
// TODO this is failing when used to raycast a box
gjk::RayHit
candybox::gjk::ShapeCast(const ShapeCastInput* input)
{
	RayHit output{};

	const DistanceProxy* proxyA = &input->proxyA;
	const DistanceProxy* proxyB = &input->proxyB;

	float radius = proxyA->radius + proxyB->radius;

	Xf2d xfA = input->transformA;
	Xf2d xfB = input->transformB;

	glm::vec2 r = input->translationB;
	glm::vec2 n = {0.f, 0.f};
	float lambda = 0.0f;
	float maxFraction = input->maxFraction;

	// Initial simplex
	Simplex simplex;
	simplex.count_ = 0;

	// Get simplex vertices as an array.
	SimplexVertex* vertices[] = {&simplex.v1_, &simplex.v2_, &simplex.v3_};

	// Get support point in -r direction
	int32_t indexA = FindSupport(proxyA, InvRotateVector(xfA.q, m::negv(r)));
	glm::vec2 wA = TransformPoint(xfA, proxyA->vertices[indexA]);
	int32_t indexB = FindSupport(proxyB, InvRotateVector(xfB.q, r));
	glm::vec2 wB = TransformPoint(xfB, proxyB->vertices[indexB]);
	glm::vec2 v = m::subv(wA, wB);

	// Sigma is the target distance between proxies
	const float sigma = m::max(LINEAR_SLOP, radius - LINEAR_SLOP);

	// Main iteration loop.
	const int32_t k_maxIters = 20;
	int32_t iter = 0;
	while (iter < k_maxIters && m::lenv(v) > sigma)
	{
		candybox_assert(simplex.count_ < 3);

		output.iterations += 1;

		// Support in direction -v (A - B)
		indexA = FindSupport(proxyA, InvRotateVector(xfA.q, m::negv(v)));
		wA = TransformPoint(xfA, proxyA->vertices[indexA]);
		indexB = FindSupport(proxyB, InvRotateVector(xfB.q, v));
		wB = TransformPoint(xfB, proxyB->vertices[indexB]);
		glm::vec2 p = m::subv(wA, wB);

		// -v is a normal at p
		v = m::normv(v);

		// Intersect ray with plane
		float vp = m::dotv(v, p);
		float vr = m::dotv(v, r);
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
		vertex->w = m::subv(vertex->wB, vertex->wA);
		vertex->a = 1.0f;
		simplex.count_ += 1;

		switch (simplex.count_)
		{
			case 1: break;

			case 2: simplex.Solve2(); break;

			case 3: simplex.Solve3(); break;

			default: candybox_assert(0);
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
	glm::vec2 pointA, pointB;
	simplex.ComputeWitnessPoints(&pointB, &pointA);

	if (m::dotv(v, v) > 0.0f) { n = m::normv(m::negv(v)); }

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
	glm::vec2 localPoint;
	glm::vec2 axis;
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
		candybox_assert(0 < count && count < 3);

		f.sweepA = *sweepA;
		f.sweepB = *sweepB;

		gjk::Xf2d xfA = GetSweepTransform(sweepA, t1);
		gjk::Xf2d xfB = GetSweepTransform(sweepB, t1);

		if (count == 1)
		{
			f.type = SepType::kPoint;
			glm::vec2 localPointA = proxyA->vertices[cache->indexA[0]];
			glm::vec2 localPointB = proxyB->vertices[cache->indexB[0]];
			glm::vec2 pointA = TransformPoint(xfA, localPointA);
			glm::vec2 pointB = TransformPoint(xfB, localPointB);
			f.axis = m::normv(m::subv(pointB, pointA));
			f.localPoint = {0.f, 0.f};
			return f;
		}

		if (cache->indexA[0] == cache->indexA[1])
		{
			// Two points on B and one on A.
			f.type = SepType::kFaceB;
			glm::vec2 localPointB1 = proxyB->vertices[cache->indexB[0]];
			glm::vec2 localPointB2 = proxyB->vertices[cache->indexB[1]];

			f.axis = m::crossvs(m::subv(localPointB2, localPointB1), 1.0f);
			f.axis = m::normv(f.axis);
			glm::vec2 normal = RotateVector(xfB.q, f.axis);

			f.localPoint = {
			    0.5f * (localPointB1.x + localPointB2.x),
			    0.5f * (localPointB1.y + localPointB2.y)};
			glm::vec2 pointB = TransformPoint(xfB, f.localPoint);

			glm::vec2 localPointA = proxyA->vertices[cache->indexA[0]];
			glm::vec2 pointA = TransformPoint(xfA, localPointA);

			float s = m::dotv(m::subv(pointA, pointB), normal);
			if (s < 0.0f) { f.axis = m::negv(f.axis); }
			return f;
		}

		// Two points on A and one or two points on B.
		f.type = SepType::kFaceA;
		glm::vec2 localPointA1 = proxyA->vertices[cache->indexA[0]];
		glm::vec2 localPointA2 = proxyA->vertices[cache->indexA[1]];

		f.axis = m::crossvs(m::subv(localPointA2, localPointA1), 1.0f);
		f.axis = m::normv(f.axis);
		glm::vec2 normal = RotateVector(xfA.q, f.axis);

		f.localPoint = {
		    0.5f * (localPointA1.x + localPointA2.x),
		    0.5f * (localPointA1.y + localPointA2.y)};
		glm::vec2 pointA = TransformPoint(xfA, f.localPoint);

		glm::vec2 localPointB = proxyB->vertices[cache->indexB[0]];
		glm::vec2 pointB = TransformPoint(xfB, localPointB);

		float s = m::dotv(m::subv(pointB, pointA), normal);
		if (s < 0.0f) { f.axis = m::negv(f.axis); }
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
			glm::vec2 axisA = InvRotateVector(xfA.q, f->axis);
			glm::vec2 axisB = InvRotateVector(xfB.q, m::negv(f->axis));

			*indexA = FindSupport(f->proxyA, axisA);
			*indexB = FindSupport(f->proxyB, axisB);

			glm::vec2 localPointA = f->proxyA->vertices[*indexA];
			glm::vec2 localPointB = f->proxyB->vertices[*indexB];

			glm::vec2 pointA = TransformPoint(xfA, localPointA);
			glm::vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = m::dotv(m::subv(pointB, pointA), f->axis);
			return separation;
		}

		case SepType::kFaceA: {
			glm::vec2 normal = RotateVector(xfA.q, f->axis);
			glm::vec2 pointA = TransformPoint(xfA, f->localPoint);

			glm::vec2 axisB = InvRotateVector(xfB.q, m::negv(normal));

			*indexA = -1;
			*indexB = FindSupport(f->proxyB, axisB);

			glm::vec2 localPointB = f->proxyB->vertices[*indexB];
			glm::vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = m::dotv(m::subv(pointB, pointA), normal);
			return separation;
		}

		case SepType::kFaceB: {
			glm::vec2 normal = RotateVector(xfB.q, f->axis);
			glm::vec2 pointB = TransformPoint(xfB, f->localPoint);

			glm::vec2 axisA = InvRotateVector(xfA.q, m::negv(normal));

			*indexB = -1;
			*indexA = FindSupport(f->proxyA, axisA);

			glm::vec2 localPointA = f->proxyA->vertices[*indexA];
			glm::vec2 pointA = TransformPoint(xfA, localPointA);

			float separation = m::dotv(m::subv(pointA, pointB), normal);
			return separation;
		}

		default:
			candybox_assert(0);
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
			glm::vec2 localPointA = f->proxyA->vertices[indexA];
			glm::vec2 localPointB = f->proxyB->vertices[indexB];

			glm::vec2 pointA = TransformPoint(xfA, localPointA);
			glm::vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = m::dotv(m::subv(pointB, pointA), f->axis);
			return separation;
		}

		case SepType::kFaceA: {
			glm::vec2 normal = RotateVector(xfA.q, f->axis);
			glm::vec2 pointA = TransformPoint(xfA, f->localPoint);

			glm::vec2 localPointB = f->proxyB->vertices[indexB];
			glm::vec2 pointB = TransformPoint(xfB, localPointB);

			float separation = m::dotv(m::subv(pointB, pointA), normal);
			return separation;
		}

		case SepType::kFaceB: {
			glm::vec2 normal = RotateVector(xfB.q, f->axis);
			glm::vec2 pointB = TransformPoint(xfB, f->localPoint);

			glm::vec2 localPointA = f->proxyA->vertices[indexA];
			glm::vec2 pointA = TransformPoint(xfA, localPointA);

			float separation = m::dotv(m::subv(pointA, pointB), normal);
			return separation;
		}

		default: candybox_assert(0); return 0.0f;
	}
}

// CCD via the local separating axis method. This seeks progression
// by computing the largest time at which separation is maintained.
gjk::TOIOutput
candybox::gjk::TimeOfImpact(const TOIInput* input)
{
	TOIOutput output;
	output.state = TOIState::kUnknown;
	output.t = input->tMax;

	const DistanceProxy* proxyA = &input->proxyA;
	const DistanceProxy* proxyB = &input->proxyB;

	Sweep sweepA = input->sweepA;
	Sweep sweepB = input->sweepB;

	// Large rotations can make the root finder fail, so normalize the sweep angles.
	float twoPi = 2.0f * m::pi;
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
	float target = m::max(LINEAR_SLOP, totalRadius + LINEAR_SLOP);
	float tolerance = 0.25f * LINEAR_SLOP;
	candybox_assert(target > tolerance);

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

				if (m::abs(s - target) < tolerance)
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
candybox::gjk::PointInCircle(glm::vec2 point, const Circle* shape)
{
	glm::vec2 center = shape->point;
	return m::distsqv(point, center) <= shape->radius * shape->radius;
}

bool
candybox::gjk::PointInCapsule(glm::vec2 point, const Capsule* shape)
{
	float rr = shape->radius * shape->radius;
	glm::vec2 p1 = shape->point1;
	glm::vec2 p2 = shape->point2;

	glm::vec2 d = m::subv(p2, p1);
	float dd = m::dotv(d, d);
	if (dd == 0.0f)
	{
		// Capsule is really a circle
		return m::distsqv(point, p1) <= rr;
	}

	// Get closest point on capsule segment
	// c = p1 + t * d
	// dot(point - c, d) = 0
	// dot(point - p1 - t * d, d) = 0
	// t = dot(point - p1, d) / dot(d, d)
	float t = m::dotv(m::subv(point, p1), d) / dd;
	t = m::clamp(t, 0.0f, 1.0f);
	glm::vec2 c = m::muladdv(p1, t, d);

	// Is query point within radius around closest point?
	return m::distsqv(point, c) <= rr;
}

bool
candybox::gjk::PointInPolygon(glm::vec2 point, const Polygon* shape)
{
	DistanceInput input{};
	input.proxyA = MakeProxy(shape->vertices, shape->count, 0.0f);
	input.proxyB = MakeProxy(&point, 1, 0.0f);
	input.transformA.setIdentity();
	input.transformB.setIdentity();
	input.useRadii = false;

	DistanceCache cache{};
	DistanceOutput output = ShapeDistance(&cache, &input);

	return output.distance <= shape->radius;
}

Box
candybox::gjk::ComputeCircleAABB(const Circle* shape, const Xf2d& xf)
{
	glm::vec2 p = TransformPoint(xf, shape->point);
	float r = shape->radius;

	Box aabb = {{p.x - r, p.y - r}, {p.x + r, p.y + r}};
	return aabb;
}

Box
candybox::gjk::ComputeCapsuleAABB(const Capsule* shape, const Xf2d& xf)
{
	glm::vec2 v1 = TransformPoint(xf, shape->point1);
	glm::vec2 v2 = TransformPoint(xf, shape->point2);

	glm::vec2 r = {shape->radius, shape->radius};
	glm::vec2 lower = m::subv(m::minv(v1, v2), r);
	glm::vec2 upper = m::addv(m::maxv(v1, v2), r);

	Box aabb = {lower, upper};
	return aabb;
}

Box
candybox::gjk::ComputePolygonAABB(const Polygon* shape, const Xf2d& xf)
{
	candybox_assert(shape->count > 0);
	glm::vec2 lower = TransformPoint(xf, shape->vertices[0]);
	glm::vec2 upper = lower;

	for (int32_t i = 1; i < shape->count; ++i)
	{
		glm::vec2 v = TransformPoint(xf, shape->vertices[i]);
		lower = m::minv(lower, v);
		upper = m::maxv(upper, v);
	}

	glm::vec2 r = {shape->radius, shape->radius};
	lower = m::subv(lower, r);
	upper = m::addv(upper, r);

	Box aabb = {lower, upper};
	return aabb;
}

Box
candybox::gjk::ComputeSegmentAABB(const Segment* shape, const Xf2d& xf)
{
	glm::vec2 v1 = TransformPoint(xf, shape->p1);
	glm::vec2 v2 = TransformPoint(xf, shape->p2);

	glm::vec2 lower = m::minv(v1, v2);
	glm::vec2 upper = m::maxv(v1, v2);

	Box aabb = {lower, upper};
	return aabb;
}

// quickhull recursion
static gjk::Hull
RecurseHull(glm::vec2 p1, glm::vec2 p2, glm::vec2* ps, int32_t count)
{
	gjk::Hull hull;
	hull.count = 0;

	if (count == 0) { return hull; }

	// create an edge vector pointing from p1 to p2
	glm::vec2 e = m::normv(m::subv(p2, p1));

	// discard points left of e and find point furthest to the right of e
	glm::vec2 rightPoints[candybox::gjk::MAX_POLY_VERTS];
	int32_t rightCount = 0;

	int32_t bestIndex = 0;
	float bestDistance = m::crossv(m::subv(ps[bestIndex], p1), e);
	if (bestDistance > 0.0f) { rightPoints[rightCount++] = ps[bestIndex]; }

	for (int32_t i = 1; i < count; ++i)
	{
		float distance = m::crossv(m::subv(ps[i], p1), e);
		if (distance > bestDistance)
		{
			bestIndex = i;
			bestDistance = distance;
		}

		if (distance > 0.0f) { rightPoints[rightCount++] = ps[i]; }
	}

	if (bestDistance < 2.0f * candybox::gjk::LINEAR_SLOP) { return hull; }

	glm::vec2 bestPoint = ps[bestIndex];

	// compute hull to the right of p1-bestPoint
	gjk::Hull hull1 = RecurseHull(p1, bestPoint, rightPoints, rightCount);

	// compute hull to the right of bestPoint-p2
	gjk::Hull hull2 = RecurseHull(bestPoint, p2, rightPoints, rightCount);

	// stitch together hulls
	for (int32_t i = 0; i < hull1.count; ++i) { hull.points[hull.count++] = hull1.points[i]; }

	hull.points[hull.count++] = bestPoint;

	for (int32_t i = 0; i < hull2.count; ++i) { hull.points[hull.count++] = hull2.points[i]; }

	candybox_assert(hull.count < candybox::gjk::MAX_POLY_VERTS);

	return hull;
}

// quickhull algorithm
// - merges vertices based on LINEAR_SLOP
// - removes collinear points using LINEAR_SLOP
// - returns an empty hull if it fails
gjk::Hull
candybox::gjk::ComputeHull(const glm::vec2* points, int32_t count)
{
	Hull hull;
	hull.count = 0;

	if (count < 3 || count > MAX_POLY_VERTS)
	{
		// check your data
		return hull;
	}

	count = m::min(count, MAX_POLY_VERTS);

	Box aabb = {{FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX}};

	// Perform aggressive point welding. First point always remains.
	// Also compute the bounding box for later.
	glm::vec2 ps[MAX_POLY_VERTS];
	int32_t n = 0;
	const float tolSqr = 16.0f * LINEAR_SLOP * LINEAR_SLOP;
	for (int32_t i = 0; i < count; ++i)
	{
		aabb.l = m::minv(aabb.l, points[i]);
		aabb.u = m::maxv(aabb.u, points[i]);

		glm::vec2 vi = points[i];

		bool unique = true;
		for (int32_t j = 0; j < i; ++j)
		{
			glm::vec2 vj = points[j];

			float distSqr = m::distsqv(vi, vj);
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
	glm::vec2 c = aabb.center();
	int32_t f1 = 0;
	float dsq1 = m::distsqv(c, ps[f1]);
	for (int32_t i = 1; i < n; ++i)
	{
		float dsq = m::distsqv(c, ps[i]);
		if (dsq > dsq1)
		{
			f1 = i;
			dsq1 = dsq;
		}
	}

	// remove p1 from working set
	glm::vec2 p1 = ps[f1];
	ps[f1] = ps[n - 1];
	n = n - 1;

	int32_t f2 = 0;
	float dsq2 = m::distsqv(p1, ps[f2]);
	for (int32_t i = 1; i < n; ++i)
	{
		float dsq = m::distsqv(p1, ps[i]);
		if (dsq > dsq2)
		{
			f2 = i;
			dsq2 = dsq;
		}
	}

	// remove p2 from working set
	glm::vec2 p2 = ps[f2];
	ps[f2] = ps[n - 1];
	n = n - 1;

	// split the points into points that are left and right of the line p1-p2.
	glm::vec2 rightPoints[MAX_POLY_VERTS - 2];
	int32_t rightCount = 0;

	glm::vec2 leftPoints[MAX_POLY_VERTS - 2];
	int32_t leftCount = 0;

	glm::vec2 e = m::normv(m::subv(p2, p1));

	for (int32_t i = 0; i < n; ++i)
	{
		float d = m::crossv(m::subv(ps[i], p1), e);

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

	candybox_assert(hull.count <= MAX_POLY_VERTS);

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

			glm::vec2 s1 = hull.points[i1];
			glm::vec2 s2 = hull.points[i2];
			glm::vec2 s3 = hull.points[i3];

			// unit edge vector for s1-s3
			glm::vec2 r = m::normv(m::subv(s3, s1));

			float distance = m::crossv(m::subv(s2, s1), r);
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
candybox::gjk::ValidateHull(const Hull* hull)
{
	if (hull->count < 3 || MAX_POLY_VERTS < hull->count) { return false; }

	// test that every point is behind every edge
	for (int32_t i = 0; i < hull->count; ++i)
	{
		// create an edge vector
		int32_t i1 = i;
		int32_t i2 = i < hull->count - 1 ? i1 + 1 : 0;
		glm::vec2 p = hull->points[i1];
		glm::vec2 e = m::normv(m::subv(hull->points[i2], p));

		for (int32_t j = 0; j < hull->count; ++j)
		{
			// skip points that subtend the current edge
			if (j == i1 || j == i2) { continue; }

			float distance = m::crossv(m::subv(hull->points[j], p), e);
			if (distance >= 0.0f) { return false; }
		}
	}

	// test for collinear points
	for (int32_t i = 0; i < hull->count; ++i)
	{
		int32_t i1 = i;
		int32_t i2 = (i + 1) % hull->count;
		int32_t i3 = (i + 2) % hull->count;

		glm::vec2 p1 = hull->points[i1];
		glm::vec2 p2 = hull->points[i2];
		glm::vec2 p3 = hull->points[i3];

		glm::vec2 e = m::normv(m::subv(p3, p1));

		float distance = m::crossv(m::subv(p2, p1), e);
		if (distance <= LINEAR_SLOP)
		{
			// p1-p2-p3 are collinear
			return false;
		}
	}

	return true;
}

gjk::RayCast::RayCast(glm::vec2 start, glm::vec2 end, float maxDist, float r)
    : p1(start), p2(end), maxFraction(maxDist), radius(r)
{
}
