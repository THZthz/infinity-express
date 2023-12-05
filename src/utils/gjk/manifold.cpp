#include "utils/GJK.hpp"
using namespace ie;

#define MAKE_ID(A, B) ((uint8_t)(A) << 8 | (uint8_t)(B))

gjk::Manifold
ie::gjk::GJK_Circles(const Circle* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd)
{
	Manifold manifold{};

	vec2 pointA = TransformPoint(xfA, A->point);
	vec2 pointB = TransformPoint(xfB, B->point);

	float distance;
	vec2 normal = NormV(SubV(pointB, pointA), distance);

	float rA = A->radius;
	float rB = B->radius;

	float separation = distance - rA - rB;
	if (separation > maxd) { return manifold; }

	vec2 cA = MulAddV(pointA, rA, normal);
	vec2 cB = MulAddV(pointB, -rB, normal);
	manifold.normal = normal;
	manifold.points[0].point = LerpV(cA, cB, 0.5f);
	manifold.points[0].separation = separation;
	manifold.points[0].id = 0;
	manifold.pointCount = 1;
	return manifold;
}

/// Compute the collision manifold between a capsule and circle
gjk::Manifold
ie::gjk::
    GJK_Capsule2Circle(const Capsule* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd)
{
	Manifold manifold{};

	// Compute circle position in the frame of the capsule.
	vec2 pB = InvTransformPoint(xfA, TransformPoint(xfB, B->point));

	// Compute closest point
	vec2 p1 = A->point1;
	vec2 p2 = A->point2;

	vec2 e = SubV(p2, p1);

	// dot(p - pA, e) = 0
	// dot(p - (p1 + s1 * e), e) = 0
	// s1 = dot(p - p1, e)
	vec2 pA;
	float s1 = DotV(SubV(pB, p1), e);
	float s2 = DotV(SubV(p2, pB), e);
	if (s1 < 0.0f)
	{
		// p1 region
		pA = p1;
	}
	else if (s2 < 0.0f)
	{
		// p2 region
		pA = p2;
	}
	else
	{
		// circle colliding with segment interior
		float s = s1 / DotV(e, e);
		pA = MulAddV(p1, s, e);
	}

	float distance;
	vec2 normal = NormV(SubV(pB, pA), distance);

	float rA = A->radius;
	float rB = B->radius;
	float separation = distance - rA - rB;
	if (separation > maxd) { return manifold; }

	vec2 cA = MulAddV(pA, rA, normal);
	vec2 cB = MulAddV(pB, -rB, normal);
	manifold.normal = RotateVector(xfA.q, normal);
	manifold.points[0].point = TransformPoint(xfA, LerpV(cA, cB, 0.5f));
	manifold.points[0].separation = separation;
	manifold.points[0].id = 0;
	manifold.pointCount = 1;
	return manifold;
}

gjk::Manifold
ie::gjk::
    GJK_Poly2Circle(const Polygon* A, Xf2d xfA, const Circle* B, Xf2d xfB, float maxd)
{
	Manifold manifold{};

	// Compute circle position in the frame of the polygon.
	vec2 c = InvTransformPoint(xfA, TransformPoint(xfB, B->point));
	float radius = A->radius + B->radius;

	// Find the min separating edge.
	int32_t normalIndex = 0;
	float separation = -FLT_MAX;
	int32_t vertexCount = A->count;
	const vec2* vertices = A->vertices;
	const vec2* normals = A->normals;

	for (int32_t i = 0; i < vertexCount; ++i)
	{
		float s = DotV(normals[i], SubV(c, vertices[i]));
		if (s > separation)
		{
			separation = s;
			normalIndex = i;
		}
	}

	if (separation - radius > maxd) { return manifold; }

	// Vertices of the reference edge.
	int32_t vertIndex1 = normalIndex;
	int32_t vertIndex2 = vertIndex1 + 1 < vertexCount ? vertIndex1 + 1 : 0;
	vec2 v1 = vertices[vertIndex1];
	vec2 v2 = vertices[vertIndex2];

	// Compute barycentric coordinates
	float u1 = DotV(SubV(c, v1), SubV(v2, v1));
	float u2 = DotV(SubV(c, v2), SubV(v1, v2));

	if (u1 < 0.0f && separation > FLT_EPSILON)
	{
		// Circle center is closest to v1 and safely outside the polygon
		vec2 normal = NormV(SubV(c, v1));
		manifold.pointCount = 1;
		manifold.normal = RotateVector(xfA.q, normal);
		vec2 cA = v1;
		vec2 cB = MulAddV(c, -radius, normal);
		manifold.points[0].point = TransformPoint(xfA, LerpV(cA, cB, 0.5f));
		manifold.points[0].separation = DotV(SubV(cB, cA), normal);
		manifold.points[0].id = 0;
	}
	else if (u2 < 0.0f && separation > FLT_EPSILON)
	{
		// Circle center is closest to v2 and safely outside the polygon
		vec2 normal = NormV(SubV(c, v2));
		manifold.pointCount = 1;
		manifold.normal = RotateVector(xfA.q, normal);
		vec2 cA = v2;
		vec2 cB = MulAddV(c, -radius, normal);
		manifold.points[0].point = TransformPoint(xfA, LerpV(cA, cB, 0.5f));
		manifold.points[0].separation = DotV(SubV(cB, cA), normal);
		manifold.points[0].id = 0;
	}
	else
	{
		// Circle center is between v1 and v2. Center may be inside polygon
		vec2 normal = normals[normalIndex];
		manifold.normal = RotateVector(xfA.q, normal);

		// cA is the projection of the circle center onto to the reference edge
		vec2 cA = MulAddV(c, -DotV(SubV(c, v1), normal), normal);

		// cB is the deepest point on the circle with respect to the reference edge
		vec2 cB = MulAddV(c, -radius, normal);

		// The contact point is the midpoint in world space
		manifold.points[0].point = TransformPoint(xfA, LerpV(cA, cB, 0.5f));
		manifold.points[0].separation = separation - radius;
		manifold.points[0].id = 0;
		manifold.pointCount = 1;
	}

	return manifold;
}

gjk::Manifold
ie::gjk::GJK_Capsules(
    const gjk::Capsule* A,
    gjk::Xf2d xfA,
    const gjk::Capsule* B,
    gjk::Xf2d xfB,
    float maxd,
    gjk::DistanceCache* cache)
{
	Polygon polyA = MakeCapsule(A->point1, A->point2, A->radius);
	Polygon polyB = MakeCapsule(B->point1, B->point2, B->radius);
	return GJK_Polygons(&polyA, xfA, &polyB, xfB, maxd, cache);
}

gjk::Manifold
ie::gjk::GJK_Seg2Capsule(
    const gjk::Segment* A,
    gjk::Xf2d xfA,
    const gjk::Capsule* B,
    gjk::Xf2d xfB,
    float maxd,
    gjk::DistanceCache* cache)
{
	Polygon polyA = MakeCapsule(A->p1, A->p2, 0.0f);
	Polygon polyB = MakeCapsule(B->point1, B->point2, B->radius);
	return GJK_Polygons(&polyA, xfA, &polyB, xfB, maxd, cache);
}

gjk::Manifold
ie::gjk::GJK_Poly2Capsule(
    const gjk::Polygon* A,
    gjk::Xf2d xfA,
    const gjk::Capsule* B,
    gjk::Xf2d xfB,
    float maxd,
    gjk::DistanceCache* cache)
{
	Polygon polyB = MakeCapsule(B->point1, B->point2, B->radius);
	return GJK_Polygons(A, xfA, &polyB, xfB, maxd, cache);
}

// Polygon clipper used by GJK and SAT to compute contact points when there are potentially two contact points.
static gjk::Manifold
PolygonClip(
    const gjk::Polygon* polyA,
    gjk::Xf2d xfA,
    const gjk::Polygon* polyB,
    gjk::Xf2d xfB,
    int32_t edgeA,
    int32_t edgeB,
    float maxd,
    bool flip)
{
	gjk::Manifold manifold{};

	// reference polygon
	const gjk::Polygon* poly1;
	int32_t i11, i12;

	// incident polygon
	const gjk::Polygon* poly2;
	int32_t i21, i22;

	gjk::Xf2d xf;

	if (flip)
	{
		poly1 = polyB;
		poly2 = polyA;
		// take points in frame A into frame B
		xf = InvMulTransforms(xfB, xfA);
		i11 = edgeB;
		i12 = edgeB + 1 < polyB->count ? edgeB + 1 : 0;
		i21 = edgeA;
		i22 = edgeA + 1 < polyA->count ? edgeA + 1 : 0;
	}
	else
	{
		poly1 = polyA;
		poly2 = polyB;
		// take points in frame B into frame A
		xf = InvMulTransforms(xfA, xfB);
		i11 = edgeA;
		i12 = edgeA + 1 < polyA->count ? edgeA + 1 : 0;
		i21 = edgeB;
		i22 = edgeB + 1 < polyB->count ? edgeB + 1 : 0;
	}

	vec2 normal = poly1->normals[i11];

	// Reference edge vertices
	vec2 v11 = poly1->vertices[i11];
	vec2 v12 = poly1->vertices[i12];

	// Incident edge vertices
	vec2 v21 = TransformPoint(xf, poly2->vertices[i21]);
	vec2 v22 = TransformPoint(xf, poly2->vertices[i22]);

	vec2 tangent = CrossSV(1.0f, normal);

	float lower1 = 0.0f;
	float upper1 = DotV(SubV(v12, v11), tangent);

	// Incident edge points opposite of tangent due to CCW winding
	float upper2 = DotV(SubV(v21, v11), tangent);
	float lower2 = DotV(SubV(v22, v11), tangent);

	// This check can fail slightly due to mismatch with GJK code.
	// Perhaps fallback to a single point here? Otherwise we get two coincident points.
	// if (upper2 < lower1 || upper1 < lower2)
	//{
	//	// numeric failure
	//	assert(false);
	//	return manifold;
	//}

	vec2 vLower;
	if (lower2 < lower1 && upper2 - lower2 > FLT_EPSILON)
	{
		vLower = LerpV(v22, v21, (lower1 - lower2) / (upper2 - lower2));
	}
	else { vLower = v22; }

	vec2 vUpper;
	if (upper2 > upper1 && upper2 - lower2 > FLT_EPSILON)
	{
		vUpper = LerpV(v22, v21, (upper1 - lower2) / (upper2 - lower2));
	}
	else { vUpper = v21; }

	// TODO_ERIN vLower can be very close to vUpper, reduce to one point?

	float separationLower = DotV(SubV(vLower, v11), normal);
	float separationUpper = DotV(SubV(vUpper, v11), normal);

	float r1 = poly1->radius;
	float r2 = poly2->radius;

	// Put contact points at midpoint, accounting for radii
	vLower = MulAddV(vLower, 0.5f * (r1 - r2 - separationLower), normal);
	vUpper = MulAddV(vUpper, 0.5f * (r1 - r2 - separationUpper), normal);

	float radius = r1 + r2;

	if (!flip)
	{
		manifold.normal = RotateVector(xfA.q, normal);
		gjk::ManifoldPoint* cp = manifold.points + 0;

		if (separationLower <= radius + maxd)
		{
			cp->point = TransformPoint(xfA, vLower);
			cp->separation = separationLower - radius;
			cp->id = MAKE_ID(i11, i22);
			manifold.pointCount += 1;
			cp += 1;
		}

		if (separationUpper <= radius + maxd)
		{
			cp->point = TransformPoint(xfA, vUpper);
			cp->separation = separationUpper - radius;
			cp->id = MAKE_ID(i12, i21);
			manifold.pointCount += 1;
		}
	}
	else
	{
		manifold.normal = RotateVector(xfB.q, NegV(normal));
		gjk::ManifoldPoint* cp = manifold.points + 0;

		if (separationUpper <= radius + maxd)
		{
			cp->point = TransformPoint(xfB, vUpper);
			cp->separation = separationUpper - radius;
			cp->id = MAKE_ID(i21, i12);
			manifold.pointCount += 1;
			cp += 1;
		}

		if (separationLower <= radius + maxd)
		{
			cp->point = TransformPoint(xfB, vLower);
			cp->separation = separationLower - radius;
			cp->id = MAKE_ID(i22, i11);
			manifold.pointCount += 1;
		}
	}

	return manifold;
}

// Find the max separation between poly1 and poly2 using edge normals from poly1.
static float
FindMaxSeparation(
    int32_t* edgeIndex,
    const gjk::Polygon* poly1,
    gjk::Xf2d xf1,
    const gjk::Polygon* poly2,
    gjk::Xf2d xf2)
{
	int32_t count1 = poly1->count;
	int32_t count2 = poly2->count;
	const vec2* n1s = poly1->normals;
	const vec2* v1s = poly1->vertices;
	const vec2* v2s = poly2->vertices;
	gjk::Xf2d xf = InvMulTransforms(xf2, xf1);

	int32_t bestIndex = 0;
	float maxSeparation = -FLT_MAX;
	for (int32_t i = 0; i < count1; ++i)
	{
		// Get poly1 normal in frame2.
		vec2 n = RotateVector(xf.q, n1s[i]);
		vec2 v1 = TransformPoint(xf, v1s[i]);

		// Find deepest point for normal i.
		float si = FLT_MAX;
		for (int32_t j = 0; j < count2; ++j)
		{
			float sij = DotV(n, SubV(v2s[j], v1));
			if (sij < si) { si = sij; }
		}

		if (si > maxSeparation)
		{
			maxSeparation = si;
			bestIndex = i;
		}
	}

	*edgeIndex = bestIndex;
	return maxSeparation;
}

// This function assumes there is overlap
static gjk::Manifold
PolygonSAT(
    const gjk::Polygon* polyA,
    gjk::Xf2d xfA,
    const gjk::Polygon* polyB,
    gjk::Xf2d xfB,
    float maxd)
{
	int32_t edgeA = 0;
	float separationA = FindMaxSeparation(&edgeA, polyA, xfA, polyB, xfB);

	int32_t edgeB = 0;
	float separationB = FindMaxSeparation(&edgeB, polyB, xfB, polyA, xfA);

	bool flip;

	if (separationB > separationA)
	{
		flip = true;
		vec2 normal = RotateVector(xfB.q, polyB->normals[edgeB]);
		vec2 searchDirection = InvRotateVector(xfA.q, normal);

		// Find the incident edge on polyA
		int32_t count = polyA->count;
		const vec2* normals = polyA->normals;
		edgeA = 0;
		float minDot = FLT_MAX;
		for (int32_t i = 0; i < count; ++i)
		{
			float dot = DotV(searchDirection, normals[i]);
			if (dot < minDot)
			{
				minDot = dot;
				edgeA = i;
			}
		}
	}
	else
	{
		flip = false;
		vec2 normal = RotateVector(xfA.q, polyA->normals[edgeA]);
		vec2 searchDirection = InvRotateVector(xfB.q, normal);

		// Find the incident edge on polyB
		int32_t count = polyB->count;
		const vec2* normals = polyB->normals;
		edgeB = 0;
		float minDot = FLT_MAX;
		for (int32_t i = 0; i < count; ++i)
		{
			float dot = DotV(searchDirection, normals[i]);
			if (dot < minDot)
			{
				minDot = dot;
				edgeB = i;
			}
		}
	}

	return PolygonClip(polyA, xfA, polyB, xfB, edgeA, edgeB, maxd, flip);
}

// Due to speculation, every polygon is rounded
// Algorithm:
// compute distance
// if distance <= 0.1f * LINEAR_SLOP
//   SAT
// else
//   find closest features from GJK
//   expect 2-1 or 1-1 or 1-2 features
//   if 2-1 or 1-2
//     clip
//   else
//     vertex-vertex
//   end
// end
gjk::Manifold
ie::gjk::GJK_Polygons(
    const gjk::Polygon* A,
    gjk::Xf2d xfA,
    const gjk::Polygon* B,
    gjk::Xf2d xfB,
    const float maxd,
    gjk::DistanceCache* cache)
{
	Manifold manifold{};
	float radius = A->radius + B->radius;

	DistanceInput input;
	input.proxyA = MakeProxy(A->vertices, A->count, 0.0f);
	input.proxyB = MakeProxy(B->vertices, B->count, 0.0f);
	input.transformA = xfA;
	input.transformB = xfB;
	input.useRadii = false;

	DistanceOutput output = ShapeDistance(cache, &input);

	if (output.distance > radius + maxd) { return manifold; }

	if (output.distance < 0.1f * LINEAR_SLOP)
	{
		// distance is small or zero, fallback to SAT
		return PolygonSAT(A, xfA, B, xfB, maxd);
	}

	if (cache->count == 1)
	{
		// vertex-vertex collision
		vec2 pA = output.pointA;
		vec2 pB = output.pointB;

		float distance = output.distance;
		manifold.normal = NormV(SubV(pB, pA));
		ManifoldPoint* cp = manifold.points + 0;
		cp->point = MulAddV(pB, 0.5f * (A->radius - B->radius - distance), manifold.normal);
		cp->separation = distance - radius;
		cp->id = MAKE_ID(cache->indexA[0], cache->indexB[0]);
		manifold.pointCount = 1;
		return manifold;
	}

	// vertex-edge collision
	assert(cache->count == 2);
	bool flip;
	int32_t countA = A->count;
	int32_t countB = B->count;
	int32_t edgeA, edgeB;

	int32_t a1 = cache->indexA[0];
	int32_t a2 = cache->indexA[1];
	int32_t b1 = cache->indexB[0];
	int32_t b2 = cache->indexB[1];

	if (a1 == a2)
	{
		// 1 point on A, expect 2 points on B
		assert(b1 != b2);

		// Find reference edge that most aligns with vector between closest points.
		// This works for capsules and polygons
		vec2 axis = InvRotateVector(xfB.q, SubV(output.pointA, output.pointB));
		float dot1 = DotV(axis, B->normals[b1]);
		float dot2 = DotV(axis, B->normals[b2]);
		edgeB = dot1 > dot2 ? b1 : b2;

		flip = true;

		// Get the normal of the reference edge in polyA's frame.
		axis = InvRotateVector(xfA.q, RotateVector(xfB.q, B->normals[edgeB]));

		// Find the incident edge on polyA
		// Limit search to edges adjacent to closest vertex on A
		int32_t edgeA1 = a1;
		int32_t edgeA2 = edgeA1 == 0 ? countA - 1 : edgeA1 - 1;
		dot1 = DotV(axis, A->normals[edgeA1]);
		dot2 = DotV(axis, A->normals[edgeA2]);
		edgeA = dot1 < dot2 ? edgeA1 : edgeA2;
	}
	else
	{
		// Find reference edge that most aligns with vector between closest points.
		// This works for capsules and polygons
		vec2 axis = InvRotateVector(xfA.q, SubV(output.pointB, output.pointA));
		float dot1 = DotV(axis, A->normals[a1]);
		float dot2 = DotV(axis, A->normals[a2]);
		edgeA = dot1 > dot2 ? a1 : a2;

		flip = false;

		// Get the normal of the reference edge in polyB's frame.
		axis = InvRotateVector(xfB.q, RotateVector(xfA.q, A->normals[edgeA]));

		// Find the incident edge on polyB
		// Limit search to edges adjacent to closest vertex
		int32_t edgeB1 = b1;
		int32_t edgeB2 = edgeB1 == 0 ? countB - 1 : edgeB1 - 1;
		dot1 = DotV(axis, B->normals[edgeB1]);
		dot2 = DotV(axis, B->normals[edgeB2]);
		edgeB = dot1 < dot2 ? edgeB1 : edgeB2;
	}

	return PolygonClip(A, xfA, B, xfB, edgeA, edgeB, maxd, flip);
}

gjk::Manifold
ie::gjk::GJK_Seg2Circle(
    const gjk::Segment* A,
    gjk::Xf2d xfA,
    const gjk::Circle* B,
    gjk::Xf2d xfB,
    float maxd)
{
	Capsule capsuleA = {A->p1, A->p2, 0.0f};
	return GJK_Capsule2Circle(&capsuleA, xfA, B, xfB, maxd);
}

gjk::Manifold
ie::gjk::GJK_Seg2Poly(
    const gjk::Segment* A,
    gjk::Xf2d xfA,
    const gjk::Polygon* B,
    gjk::Xf2d xfB,
    float maxd,
    gjk::DistanceCache* cache)
{
	Polygon polygonA = MakeCapsule(A->p1, A->p2, 0.0f);
	return GJK_Polygons(&polygonA, xfA, B, xfB, maxd, cache);
}
