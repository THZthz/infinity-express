#include <algorithm>
#include <cstring>
#include <vector>
#include "utils/GJK.hpp"
using namespace ie;

gjk::Polygon
ie::gjk::MakePolygon(const Hull *hull, float radius)
{
	assert(hull->count >= 3);

	Polygon shape;
	shape.count = hull->count;
	shape.radius = radius;

	// Copy vertices
	for (int32_t i = 0; i < shape.count; ++i) { shape.vertices[i] = hull->points[i]; }

	// Compute normals. Ensure the edges have non-zero length.
	for (int32_t i = 0; i < shape.count; ++i)
	{
		int32_t i1 = i;
		int32_t i2 = i + 1 < shape.count ? i + 1 : 0;
		vec2 edge = SubV(shape.vertices[i2], shape.vertices[i1]);
		assert(DotV(edge, edge) > FLT_EPSILON * FLT_EPSILON);
		shape.normals[i] = NormV(CrossVS(edge, 1.0f));
	}

	return shape;
}

gjk::Polygon
ie::gjk::MakePolygon(const vec2 *vertices, int count, float radius)
{
	Hull hull = ComputeHull(vertices, count);
	return MakePolygon(&hull, radius);
}

gjk::Polygon
ie::gjk::MakeSquare(float h)
{
	return MakeBox(h, h);
}

gjk::Polygon
ie::gjk::MakeBox(float hx, float hy)
{
	Polygon shape;
	shape.count = 4;
	shape.vertices[0] = {-hx, -hy};
	shape.vertices[1] = {hx, -hy};
	shape.vertices[2] = {hx, hy};
	shape.vertices[3] = {-hx, hy};
	shape.normals[0] = {0.0f, -1.0f};
	shape.normals[1] = {1.0f, 0.0f};
	shape.normals[2] = {0.0f, 1.0f};
	shape.normals[3] = {-1.0f, 0.0f};
	shape.radius = 0.0f;
	return shape;
}

gjk::Polygon
ie::gjk::MakeRoundedBox(float hx, float hy, float radius)
{
	Polygon shape = MakeBox(hx, hy);
	shape.radius = radius;
	return shape;
}

gjk::Polygon
ie::gjk::MakeOffsetBox(float hx, float hy, vec2 center, float angle)
{
	Xf2d xf;
	xf.p = center;
	xf.q.Set(angle);

	Polygon shape;
	shape.count = 4;
	shape.vertices[0] = TransformPoint(xf, {-hx, -hy});
	shape.vertices[1] = TransformPoint(xf, {hx, -hy});
	shape.vertices[2] = TransformPoint(xf, {hx, hy});
	shape.vertices[3] = TransformPoint(xf, {-hx, hy});
	shape.normals[0] = RotateVector(xf.q, {0.0f, -1.0f});
	shape.normals[1] = RotateVector(xf.q, {1.0f, 0.0f});
	shape.normals[2] = RotateVector(xf.q, {0.0f, 1.0f});
	shape.normals[3] = RotateVector(xf.q, {-1.0f, 0.0f});
	shape.radius = 0.0f;
	return shape;
}

gjk::Polygon
ie::gjk::MakeCapsule(vec2 p1, vec2 p2, float radius)
{
	Polygon shape;
	shape.vertices[0] = p1;
	shape.vertices[1] = p2;

	vec2 axis = NormV(SubV(p2, p1));
	vec2 normal = {axis.y, -axis.x}; // Right perp.

	shape.normals[0] = normal;
	shape.normals[1] = NegV(normal);
	shape.count = 2;
	shape.radius = radius;

	return shape;
}

gjk::Polygon
ie::gjk::MakeAABB(const Box &aabb)
{
	vec2 ex = aabb.extents();
	return MakeOffsetBox(ex.x, ex.y, aabb.center(), 0.f);
}

bool
ShapeProject(const vec2 *ta, uint32_t na, const vec2 *tb, uint32_t nb)
{
	for (unsigned i = 0; i < na; i++)
	{
		vec2 point = ta[i];
		vec2 nextPoint = ta[(i + 1) % na];
		vec2 edge = nextPoint - point;
		vec2 edgePerpendicular = CCW90V(edge);

		float pointProj = ProjectV(point, edgePerpendicular);
		float minRightProj = ProjectV(tb[0], edgePerpendicular);

		for (unsigned j = 1; j < nb; j++)
		{
			float proj = ProjectV(tb[j], edgePerpendicular);
			minRightProj = std::min(minRightProj, proj);
		}

		if (minRightProj > pointProj) return false;
	}

	return true;
}

bool
ie::gjk::ShapeIntersect(
    const vec2 *a,
    uint32_t na,
    const vec2 *b,
    uint32_t nb,
    const Xf2d &xfa,
    const Xf2d &xfb)
{
	// First transform points.
	std::vector<vec2> ta;
	std::vector<vec2> tb;
	ta.resize(na);
	tb.resize(nb);
	for (uint32_t i = 0; i < na; ++i) ta[i] = TransformPoint(xfa, a[i]);
	for (uint32_t i = 0; i < na; ++i) tb[i] = TransformPoint(xfb, b[i]);

	return ShapeProject(ta.data(), na, tb.data(), nb) &&
	       ShapeProject(tb.data(), nb, ta.data(), na);
}

bool
ie::gjk::ShapeIntersect(
    const gjk::Polygon &a,
    const gjk::Polygon &b,
    const gjk::Xf2d &xfa,
    const gjk::Xf2d &xfb)
{
	return ShapeIntersect(a.vertices, a.count, b.vertices, b.count, xfa, xfb);
}

bool
ie::gjk::RayIntersect(
    const vec2 &as,
    const vec2 &ad,
    const vec2 &bs,
    const vec2 &bd,
    vec2 &intersection)
{
	float dx = bs.x - as.x;
	float dy = bs.y - as.y;

	float det = bd.x * ad.y - bd.y * ad.x;
	if (det == 0.0f) return false;

	float u = (dy * bd.x - dx * bd.y) / det;
	if (u < 0.0f) return false;

	float v = (dy * ad.x - dx * ad.y) / det;
	if (v < 0.0f) return false;

	intersection = as + ad * u;

	return true;
}

bool
ie::gjk::SegmentIntersection(
    const gjk::Segment &a,
    const gjk::Segment &b,
    bool infiniteLines,
    vec2 &out)
{
	// calculate un-normalized direction vectors
	vec2 r = a.Direction(false);
	vec2 s = b.Direction(false);

	vec2 originDist = SubV(b.p1, a.p1);

	float uNumerator = CrossV(originDist, r);
	float denominator = CrossV(r, s);

	if (Abs(denominator) < 0.0001f)
	{
		// The lines are parallel
		return false;
	}

	// solve the intersection positions
	float u = uNumerator / denominator;
	float t = CrossV(originDist, s) / denominator;

	if (!infiniteLines && (t < 0 || t > 1 || u < 0 || u > 1))
	{
		// the intersection lies outside of the line segments
		return false;
	}

	// calculate the intersection point
	// a.a + r * t;
	out = AddV(a.p1, MulSV(t, r));
	return true;
}
