#include <algorithm>
#include <cstring>
#include <vector>
#include "candybox/GJK.hpp"
using namespace candybox;

gjk::Polygon
candybox::gjk::MakePolygon(const Hull *hull, float radius)
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
		glm::vec2 edge = m::subv(shape.vertices[i2], shape.vertices[i1]);
		assert(m::dotv(edge, edge) > FLT_EPSILON * FLT_EPSILON);
		shape.normals[i] = m::normv(m::crossvs(edge, 1.0f));
	}

	return shape;
}

gjk::Polygon
candybox::gjk::MakePolygon(const glm::vec2 *vertices, int count, float radius)
{
	Hull hull = ComputeHull(vertices, count);
	return MakePolygon(&hull, radius);
}

gjk::Polygon
candybox::gjk::MakeSquare(float h)
{
	return MakeBox(h, h);
}

gjk::Polygon
candybox::gjk::MakeBox(float hx, float hy)
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
candybox::gjk::MakeRoundedBox(float hx, float hy, float radius)
{
	Polygon shape = MakeBox(hx, hy);
	shape.radius = radius;
	return shape;
}

gjk::Polygon
candybox::gjk::MakeOffsetBox(float hx, float hy, glm::vec2 center, float angle)
{
	Xf2d xf;
	xf.p = center;
	xf.q.set(angle);

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
candybox::gjk::MakeCapsule(glm::vec2 p1, glm::vec2 p2, float radius)
{
	Polygon shape;
	shape.vertices[0] = p1;
	shape.vertices[1] = p2;

	glm::vec2 axis = m::normv(m::subv(p2, p1));
	glm::vec2 normal = {axis.y, -axis.x}; // Right perp.

	shape.normals[0] = normal;
	shape.normals[1] = m::negv(normal);
	shape.count = 2;
	shape.radius = radius;

	return shape;
}

gjk::Polygon
candybox::gjk::MakeAABB(const Box &aabb)
{
	glm::vec2 ex = aabb.extents();
	return MakeOffsetBox(ex.x, ex.y, aabb.center(), 0.f);
}

bool
ShapeProject(const glm::vec2 *ta, uint32_t na, const glm::vec2 *tb, uint32_t nb)
{
	for (unsigned i = 0; i < na; i++)
	{
		glm::vec2 point = ta[i];
		glm::vec2 nextPoint = ta[(i + 1) % na];
		glm::vec2 edge = nextPoint - point;
		glm::vec2 edgePerpendicular = m::ccw90v(edge);

		float pointProj = m::projectv(point, edgePerpendicular);
		float minRightProj = m::projectv(tb[0], edgePerpendicular);

		for (unsigned j = 1; j < nb; j++)
		{
			float proj = m::projectv(tb[j], edgePerpendicular);
			minRightProj = std::min(minRightProj, proj);
		}

		if (minRightProj > pointProj) return false;
	}

	return true;
}

bool
candybox::gjk::ShapeIntersect(
    const glm::vec2 *a,
    uint32_t na,
    const glm::vec2 *b,
    uint32_t nb,
    const Xf2d &xfa,
    const Xf2d &xfb)
{
	// First transform points.
	std::vector<glm::vec2> ta;
	std::vector<glm::vec2> tb;
	ta.resize(na);
	tb.resize(nb);
	for (uint32_t i = 0; i < na; ++i) ta[i] = TransformPoint(xfa, a[i]);
	for (uint32_t i = 0; i < na; ++i) tb[i] = TransformPoint(xfb, b[i]);

	return ShapeProject(ta.data(), na, tb.data(), nb) &&
	       ShapeProject(tb.data(), nb, ta.data(), na);
}

bool
candybox::gjk::ShapeIntersect(
    const gjk::Polygon &a,
    const gjk::Polygon &b,
    const gjk::Xf2d &xfa,
    const gjk::Xf2d &xfb)
{
	return ShapeIntersect(a.vertices, a.count, b.vertices, b.count, xfa, xfb);
}

bool
candybox::gjk::RayIntersect(
    const glm::vec2 &as,
    const glm::vec2 &ad,
    const glm::vec2 &bs,
    const glm::vec2 &bd,
    glm::vec2 &intersection)
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
candybox::gjk::SegmentIntersection(
    const gjk::Segment &a,
    const gjk::Segment &b,
    bool infiniteLines,
    glm::vec2 &out)
{
	// calculate un-normalized direction vectors
	glm::vec2 r = a.direction(false);
	glm::vec2 s = b.direction(false);

	glm::vec2 originDist = m::subv(b.p1, a.p1);

	float uNumerator = m::crossv(originDist, r);
	float denominator = m::crossv(r, s);

	if (m::abs(denominator) < 0.0001f)
	{
		// The lines are parallel
		return false;
	}

	// solve the intersection positions
	float u = uNumerator / denominator;
	float t = m::crossv(originDist, s) / denominator;

	if (!infiniteLines && (t < 0 || t > 1 || u < 0 || u > 1))
	{
		// the intersection lies outside of the line segments
		return false;
	}

	// calculate the intersection point
	// a.a + r * t;
	out = m::addv(a.p1, m::mulsv(t, r));
	return true;
}
