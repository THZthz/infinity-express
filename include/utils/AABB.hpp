#ifndef IE_AABB_HPP
#define IE_AABB_HPP

#include "utils/Linear.hpp"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4514)
#endif

// NOLINTBEGIN(modernize-use-nodiscard)
namespace ie {

//! \defgroup aabb
//! @{

// █████╗  █████╗ ██████╗ ██████╗
//██╔══██╗██╔══██╗██╔══██╗██╔══██╗
//███████║███████║██████╔╝██████╔╝
//██╔══██║██╔══██║██╔══██╗██╔══██╗
//██║  ██║██║  ██║██████╔╝██████╔╝
//╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚═════╝

template <typename T>
class AABB_
{
public:
	vec<2, T> l;
	vec<2, T> u;

	AABB_() : l(0, 0), u(0, 0) { }
	AABB_(const vec<2, T> &lower, const vec<2, T> &upper) : l(lower), u(upper) { }

	AABB_ Union(const AABB_ &b) const
	{
		AABB_ res = b;
		if (l.x < res.l.x) res.l.x = l.x;
		if (l.y < res.l.y) res.l.y = l.y;
		if (u.x > res.u.x) res.u.x = u.x;
		if (u.y > res.u.y) res.u.y = u.y;
		return res;
	}

	bool Contains(const AABB_ &b) const
	{
		return (l.x <= b.l.x && l.y <= b.l.y && b.u.x <= u.x && b.u.y <= u.y);
	}

	bool Contains(const vec<2, T> &p) const
	{
		return p.x <= u.x && p.x >= l.x && p.y <= u.y && p.y >= l.y;
	}

	// Test whether two aabb boxes overlaps.
	bool Overlaps(const AABB_ &b) const
	{
		vec<2, T> d1{b.l.x - u.x, b.l.y - u.y};
		vec<2, T> d2{l.x - b.u.x, l.y - b.u.y};
		if (d1.x > 0 || d1.y > 0) return false;
		if (d2.x > 0 || d2.y > 0) return false;
		return true;
	}

	void SetLower(const vec<2, T> &lowerBound) { l = lowerBound; }
	void SetUpper(const vec<2, T> &upperBound) { u = upperBound; }
	vec<2, T> GetLower() const { return l; }
	vec<2, T> GetUpper() const { return u; }

	// Expand the bounding box to contain a point.
	void Expand(vec<2, T> p)
	{
		if (p.x < l.x) l.x = p.x;
		else if (p.x > u.x) u.x = p.x;

		if (p.y < l.y) l.y = p.y;
		else if (p.y > u.y) u.y = p.y;
	}
};

using AABB64 = AABB_<int64_t>;

class AABB : public AABB_<float>
{
public:
	AABB() : AABB_() { }
	AABB(const vec2 &lower, const vec2 &upper) : AABB_(lower, upper) { }

	float Perimeter() const;
	AABB GetRotated(float rad) const;
	vec2 Center() const;
	vec2 Extents() const;
	vec2 Dims() const;
	vec2 HalfDims() const;
	void SetCenter(const vec2 &center);
	void IncCenter(const vec2 &increment);
	void SetHalfDims(const vec2 &dim);
	void SetDims(const vec2 &dim);

	// Any other way to avoid this? ;)
	explicit AABB(const AABB_<float> &rhs) : AABB_(rhs) { }
	AABB &operator=(const AABB_<float> &rhs);
};
// NOLINTEND(modernize-use-nodiscard)

inline AABB &
AABB::operator=(const AABB_<float> &rhs)
{
	l = rhs.l;
	u = rhs.u;
	return *this;
}

inline float
AABB::Perimeter() const
{
	return 2.0f * (u.x - l.x + u.y - l.y);
}

inline AABB
AABB::GetRotated(float rad) const
{
	// Get new dimensions.
	float c = Cos(rad);
	float s = Sin(rad);

	vec2 extent = Extents();
	vec2 newExtent(extent.y * s + extent.x * c, extent.x * s + extent.y * c);
	vec2 center = Center();

	return {SubV(center, newExtent), AddV(center, newExtent)};
}

inline vec2
AABB::Center() const
{
	return {0.5f * (l.x + u.x), 0.5f * (l.y + u.y)};
}

inline vec2
AABB::Extents() const
{
	return {0.5f * (u.x - l.x), 0.5f * (u.y - l.y)};
}

inline vec2
AABB::Dims() const
{
	return {u.x - l.x, u.y - l.y};
}

inline vec2
AABB::HalfDims() const
{
	return Extents();
}

inline void
AABB::SetCenter(const vec2 &center)
{
	vec2 ex = Extents();
	SetLower(SubV(center, ex));
	SetUpper(AddV(center, ex));
}

inline void
AABB::IncCenter(const vec2 &increment)
{
	vec2 center = AddV(Center(), increment);
	SetCenter(center);
}

inline void
AABB::SetHalfDims(const vec2 &dim)
{
	vec2 center = Center();
	SetLower(SubV(center, dim));
	SetUpper(AddV(center, dim));
}

inline void
AABB::SetDims(const vec2 &dim)
{
	SetHalfDims(ScaleV(dim, 0.5f));
}

//! @}

} // namespace ie

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // IE_AABB_HPP
