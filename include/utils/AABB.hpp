#ifndef IE_AABB_HPP
#define IE_AABB_HPP

#include "utils/Linear.hpp"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4514)
#endif

namespace ie {

//! \defgroup aabb
//! @{

// █████╗  █████╗ ██████╗ ██████╗
//██╔══██╗██╔══██╗██╔══██╗██╔══██╗
//███████║███████║██████╔╝██████╔╝
//██╔══██║██╔══██║██╔══██╗██╔══██╗
//██║  ██║██║  ██║██████╔╝██████╔╝
//╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚═════╝

// NOLINTBEGIN(modernize-use-nodiscard)
template <typename T>
class TBox
{
public:
	vec<2, T> l;
	vec<2, T> u;

	TBox() : l(0, 0), u(0, 0) { }
	TBox(const vec<2, T> &lower, const vec<2, T> &upper) : l(lower), u(upper) { }

	TBox combine(const TBox &b) const
	{
		TBox res = b;
		if (l.x < res.l.x) res.l.x = l.x;
		if (l.y < res.l.y) res.l.y = l.y;
		if (u.x > res.u.x) res.u.x = u.x;
		if (u.y > res.u.y) res.u.y = u.y;
		return res;
	}

	bool contains(const TBox &b) const
	{
		return (l.x <= b.l.x && l.y <= b.l.y && b.u.x <= u.x && b.u.y <= u.y);
	}

	bool contains(const vec<2, T> &p) const
	{
		return p.x <= u.x && p.x >= l.x && p.y <= u.y && p.y >= l.y;
	}

	// test whether two aabb boxes overlaps.
	bool overlaps(const TBox &b) const
	{
		vec<2, T> d1{b.l.x - u.x, b.l.y - u.y};
		vec<2, T> d2{l.x - b.u.x, l.y - b.u.y};
		if (d1.x > 0 || d1.y > 0) return false;
		if (d2.x > 0 || d2.y > 0) return false;
		return true;
	}

	void setLower(const vec<2, T> &lowerBound) { l = lowerBound; }
	void setUpper(const vec<2, T> &upperBound) { u = upperBound; }
	vec<2, T> getLower() const { return l; }
	vec<2, T> getUpper() const { return u; }

	// Expand the bounding box to contain a point.
	void expand(vec<2, T> p)
	{
		if (p.x < l.x) l.x = p.x;
		else if (p.x > u.x) u.x = p.x;

		if (p.y < l.y) l.y = p.y;
		else if (p.y > u.y) u.y = p.y;
	}
};

using Box64 = TBox<int64_t>;

class Box : public TBox<float>
{
public:
	Box() : TBox() { }
	Box(const vec2 &lower, const vec2 &upper) : TBox(lower, upper) { }

	float perimeter() const;
	Box getRotated(float rad) const;
	vec2 center() const;
	vec2 extents() const;
	vec2 dimensions() const;
	vec2 halfDimensions() const;
	void setCenter(const vec2 &center);
	void incCenter(const vec2 &increment);
	void setHalfDims(const vec2 &dim);
	void setDims(const vec2 &dim);

	// Any other way to avoid this? ;)
	explicit Box(const TBox<float> &rhs) : TBox(rhs) { }
	Box &operator=(const TBox<float> &rhs);
};
// NOLINTEND(modernize-use-nodiscard)

inline Box &
Box::operator=(const TBox<float> &rhs)
{
	l = rhs.l;
	u = rhs.u;
	return *this;
}

inline float
Box::perimeter() const
{
	return 2.0f * (u.x - l.x + u.y - l.y);
}

inline Box
Box::getRotated(float rad) const
{
	// get new dimensions
	float c = Cos(rad);
	float s = Sin(rad);
	vec2 extent = extents();
	vec2 newExtent(extent.y * s + extent.x * c, extent.x * s + extent.y * c);
	vec2 ce = center();

	return {SubV(ce, newExtent), AddV(ce, newExtent)};
}

inline vec2
Box::center() const
{
	return {0.5f * (l.x + u.x), 0.5f * (l.y + u.y)};
}

inline vec2
Box::extents() const
{
	return {0.5f * (u.x - l.x), 0.5f * (u.y - l.y)};
}

inline vec2
Box::dimensions() const
{
	return {u.x - l.x, u.y - l.y};
}

inline vec2
Box::halfDimensions() const
{
	return extents();
}

inline void
Box::setCenter(const vec2 &center)
{
	vec2 ex = extents();
	setLower(SubV(center, ex));
	setUpper(AddV(center, ex));
}

inline void
Box::incCenter(const vec2 &increment)
{
	vec2 c = AddV(center(), increment);
	setCenter(c);
}

inline void
Box::setHalfDims(const vec2 &dim)
{
	vec2 c = center();
	setLower(SubV(c, dim));
	setUpper(AddV(c, dim));
}

inline void
Box::setDims(const vec2 &dim)
{
	setHalfDims(ScaleV(dim, 0.5f));
}

//! @}

} // namespace ie

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // IE_AABB_HPP
