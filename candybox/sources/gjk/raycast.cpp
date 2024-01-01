#include "candybox/GJK.hpp"
using namespace candybox::gjk;

// Precision Improvements for Ray / Sphere Intersection - Ray Tracing Gems 2019
// http://www.codercorner.com/blog/?p=321
RayHit
candybox::gjk::RayCastCircle(const RayCast* input, const Circle* shape)
{
	glm::vec2 p = shape->point;

	RayHit output{};

	// Shift ray so circle center is the origin
	glm::vec2 s = m::subv(input->p1, p);
	float length;
	glm::vec2 d = m::normv(m::subv(input->p2, input->p1), length);
	if (length == 0.0f)
	{
		// zero length ray
		return output;
	}

	// Find closest point on ray to origin

	// solve: dot(s + t * d, d) = 0
	float t = -m::dotv(s, d);

	// c is the closest point on the line to the origin
	glm::vec2 c = m::muladdv(s, t, d);

	float cc = m::dotv(c, c);
	float r = shape->radius + input->radius;
	float rr = r * r;

	if (cc > rr)
	{
		// closest point is outside the circle
		return output;
	}

	// Pythagorus
	float h = sqrtf(rr - cc);

	float fraction = t - h;

	if (fraction < 0.0f || input->maxFraction * length < fraction)
	{
		// outside the range of the ray segment
		return output;
	}

	glm::vec2 hitPoint = m::muladdv(s, fraction, d);

	output.fraction = fraction / length;
	output.normal = m::normv(hitPoint);
	output.point = m::muladdv(p, shape->radius, output.normal);
	output.hit = true;

	return output;
}

RayHit
candybox::gjk::RayCastCapsule(const RayCast* input, const Capsule* shape)
{
	RayHit output{};

	glm::vec2 v1 = shape->point1;
	glm::vec2 v2 = shape->point2;

	glm::vec2 e = m::subv(v2, v1);

	float capsuleLength;
	glm::vec2 a = m::normv(e, capsuleLength);

	if (capsuleLength < FLT_EPSILON)
	{
		// Capsule is really a circle
		Circle circle = {v1, shape->radius};
		return RayCastCircle(input, &circle);
	}

	glm::vec2 p1 = input->p1;
	glm::vec2 p2 = input->p2;

	// Ray from capsule start to ray start
	glm::vec2 q = m::subv(p1, v1);
	float qa = m::dotv(q, a);

	// Vector to ray start that is perpendicular to capsule axis
	glm::vec2 qp = m::muladdv(q, -qa, a);

	float radius = input->radius + shape->radius;

	// Does the ray start within the infinite length capsule?
	if (m::dotv(qp, qp) < radius * radius)
	{
		if (qa < 0.0f)
		{
			// start point behind capsule segment
			Circle circle = {v1, shape->radius};
			return RayCastCircle(input, &circle);
		}

		if (qa > 1.0f)
		{
			// start point ahead of capsule segment
			Circle circle = {v2, shape->radius};
			return RayCastCircle(input, &circle);
		}

		// ray starts inside capsule -> no hit
		return output;
	}

	// Perpendicular to capsule axis, pointing right
	glm::vec2 n = {a.y, -a.x};

	float rayLength;
	glm::vec2 u = m::normv(m::subv(p2, p1), rayLength);

	// Intersect ray with infinite length capsule
	// v1 + radius * n + s1 * a = p1 + s2 * u
	// v1 - radius * n + s1 * a = p1 + s2 * u

	// s1 * a - s2 * u = b
	// b = q - radius * ap
	// or
	// b = q + radius * ap

	// Cramer's rule [a -u]
	float den = -a.x * u.y + u.x * a.y;
	if (-FLT_EPSILON < den && den < FLT_EPSILON)
	{
		// Ray is parallel to capsule and outside infinite length capsule
		return output;
	}

	glm::vec2 b1 = m::mulsubv(q, radius, n);
	glm::vec2 b2 = m::muladdv(q, radius, n);

	float invDen = 1.0f / den;

	// Cramer's rule [a b1]
	float s21 = (a.x * b1.y - b1.x * a.y) * invDen;

	// Cramer's rule [a b2]
	float s22 = (a.x * b2.y - b2.x * a.y) * invDen;

	float s2;
	glm::vec2 b;
	if (s21 < s22)
	{
		s2 = s21;
		b = b1;
	}
	else
	{
		s2 = s22;
		b = b2;
		n = m::negv(n);
	}

	if (s2 < 0.0f || input->maxFraction * rayLength < s2) { return output; }

	// Cramer's rule [b -u]
	float s1 = (-b.x * u.y + u.x * b.y) * invDen;

	if (s1 < 0.0f)
	{
		// ray passes behind capsule segment
		Circle circle = {v1, shape->radius};
		return RayCastCircle(input, &circle);
	}
	else if (capsuleLength < s1)
	{
		// ray passes ahead of capsule segment
		Circle circle = {v2, shape->radius};
		return RayCastCircle(input, &circle);
	}
	else
	{
		// ray hits capsule side
		output.fraction = s2 / rayLength;
		output.point = m::addv(m::lerpv(v1, v2, s1 / capsuleLength), m::mulsv(shape->radius, n));
		output.normal = n;
		output.hit = true;
		return output;
	}
}

// Ray vs line segment
RayHit
candybox::gjk::RayCastSegment(const RayCast* input, const Segment* shape)
{
	if (input->radius == 0.0f)
	{
		// Put the ray into the edge's frame of reference.
		glm::vec2 p1 = input->p1;
		glm::vec2 p2 = input->p2;
		glm::vec2 d = m::subv(p2, p1);

		glm::vec2 v1 = shape->p1;
		glm::vec2 v2 = shape->p2;
		glm::vec2 e = m::subv(v2, v1);

		RayHit output{};

		float length;
		glm::vec2 eUnit = m::normv(e, length);
		if (length == 0.0f) { return output; }

		// Normal points to the right, looking from v1 towards v2
		glm::vec2 normal = {eUnit.y, -eUnit.x};

		// Intersect ray with infinite segment using normal
		// Similar to intersecting a ray with an infinite plane
		// p = p1 + t * d
		// dot(normal, p - v1) = 0
		// dot(normal, p1 - v1) + t * dot(normal, d) = 0
		float numerator = m::dotv(normal, m::subv(v1, p1));
		float denominator = m::dotv(normal, d);

		if (denominator == 0.0f)
		{
			// parallel
			return output;
		}

		float t = numerator / denominator;
		if (t < 0.0f || input->maxFraction < t)
		{
			// out of ray range
			return output;
		}

		// Intersection point on infinite segment
		glm::vec2 p = m::muladdv(p1, t, d);

		// Compute position of p along segment
		// p = v1 + s * e
		// s = dot(p - v1, e) / dot(e, e)

		float s = m::dotv(m::subv(p, v1), eUnit);
		if (s < 0.0f || length < s)
		{
			// out of segment range
			return output;
		}

		if (numerator > 0.0f) { normal = m::negv(normal); }

		output.fraction = t;
		output.normal = normal;
		output.hit = true;

		return output;
	}

	Capsule capsule = {shape->p1, shape->p2, 0.0f};
	return RayCastCapsule(input, &capsule);
}

RayHit
candybox::gjk::RayCastPolygon(const RayCast* input, const Polygon* shape)
{
	if (input->radius == 0.0f && shape->radius == 0.0f)
	{
		// Put the ray into the polygon's frame of reference.
		glm::vec2 p1 = input->p1;
		glm::vec2 p2 = input->p2;
		glm::vec2 d = m::subv(p2, p1);

		float lower = 0.0f, upper = input->maxFraction;

		int32_t index = -1;

		RayHit output{};

		for (int32_t i = 0; i < shape->count; ++i)
		{
			// p = p1 + a * d
			// dot(normal, p - v) = 0
			// dot(normal, p1 - v) + a * dot(normal, d) = 0
			float numerator = m::dotv(shape->normals[i], m::subv(shape->vertices[i], p1));
			float denominator = m::dotv(shape->normals[i], d);

			if (denominator == 0.0f)
			{
				if (numerator < 0.0f) { return output; }
			}
			else
			{
				// Note: we want this predicate without division:
				// lower < numerator / denominator, where denominator < 0
				// Since denominator < 0, we have to flip the inequality:
				// lower < numerator / denominator <==> denominator * lower > numerator.
				if (denominator < 0.0f && numerator < lower * denominator)
				{
					// Increase lower.
					// The segment enters this half-space.
					lower = numerator / denominator;
					index = i;
				}
				else if (denominator > 0.0f && numerator < upper * denominator)
				{
					// Decrease upper.
					// The segment exits this half-space.
					upper = numerator / denominator;
				}
			}

			// The use of epsilon here causes the NVG_ASSERT on lower to trip
			// in some cases. Apparently the use of epsilon was to make edge
			// shapes work, but now those are handled separately.
			// if (upper < lower - b2_epsilon)
			if (upper < lower) { return output; }
		}

		assert(0.0f <= lower && lower <= input->maxFraction);

		if (index >= 0)
		{
			output.fraction = lower;
			output.normal = shape->normals[index];
			output.point = m::lerpv(p1, p2, output.fraction);
			output.hit = true;
		}

		return output;
	}

	// TODO this is not working for ray vs box (zero radii)
	ShapeCastInput castInput;
	castInput.proxyA = MakeProxy(shape->vertices, shape->count, shape->radius);
	castInput.proxyB = MakeProxy(&input->p1, 1, input->radius);
	castInput.transformA = Xf2d{{0.f, 0.f}, Rot2d{}}; // TODO identity
	castInput.transformB = Xf2d{{0.f, 0.f}, Rot2d{}};
	castInput.translationB = m::subv(input->p2, input->p1);
	castInput.maxFraction = input->maxFraction;
	return ShapeCast(&castInput);
}

bool
candybox::gjk::RaycastAABB(const Box& a, const glm::vec2& p1, const glm::vec2& p2, RayHit* output)
{
	// Radius not handled
	output->normal = {0, 0};
	output->point = {0, 0};
	output->fraction = 0.f;
	output->iterations = 0;
	output->hit = false;

	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	glm::vec2 p = p1;
	glm::vec2 d = m::subv(p2, p1);
	glm::vec2 absD = m::absv(d);

	glm::vec2 normal = {0, 0};

	// x-coordinate
	if (absD.x < FLT_EPSILON)
	{
		// parallel
		if (p.x < a.l.x || a.u.x < p.x) { return output->hit; }
	}
	else
	{
		float inv_d = 1.0f / d.x;
		float t1 = (a.l.x - p.x) * inv_d;
		float t2 = (a.u.x - p.x) * inv_d;

		// Sign of the normal vector.
		float s = -1.0f;

		if (t1 > t2)
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
			s = 1.0f;
		}

		// Push the min up
		if (t1 > tmin)
		{
			normal.y = 0.0f;
			normal.x = s;
			tmin = t1;
		}

		// Pull the max down
		tmax = m::min(tmax, t2);

		if (tmin > tmax) { return output; }
	}

	// y-coordinate
	if (absD.y < FLT_EPSILON)
	{
		// parallel
		if (p.y < a.l.y || a.u.y < p.y) { return output->hit; }
	}
	else
	{
		float inv_d = 1.0f / d.y;
		float t1 = (a.l.y - p.y) * inv_d;
		float t2 = (a.u.y - p.y) * inv_d;

		// Sign of the normal vector.
		float s = -1.0f;

		if (t1 > t2)
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
			s = 1.0f;
		}

		// Push the min up
		if (t1 > tmin)
		{
			normal.x = 0.0f;
			normal.y = s;
			tmin = t1;
		}

		// Pull the max down
		tmax = m::min(tmax, t2);

		if (tmin > tmax) { return output->hit; }
	}

	// Does the ray start inside the box?
	// Does the ray intersect beyond the max fraction?
	if (tmin < 0.0f || 1.0f < tmin) { return output->hit; }

	// Intersection.
	output->fraction = tmin;
	output->normal = normal;
	output->point = m::lerpv(p1, p2, tmin);
	output->hit = true;
	return output->hit;
}
