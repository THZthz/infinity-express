#ifndef IE_AABB_HPP
#define IE_AABB_HPP

#include <cstdint>
#include <ostream>
#include <utility>
#include "candybox/Linear.hpp"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4514)
#endif

namespace ie {
// NOLINTBEGIN(modernize-use-nodiscard)

//! \defgroup BoundingBox
//! @{

namespace detail {
enum VolumeMode
{
	eNormalVolume = 0, // Faster but can cause poor merges
	eSphericalVolume // Slower but helps certain merge cases
};

enum RegionType
{
	eNW = 0,
	eNE,
	eSW,
	eSE
};
} // namespace detail

template <typename T, int D = 2> class TBox
{
public:
	using tvec = glm::vec<D, T>;

	tvec l; // lower bound
	tvec u; // upper bound

	TBox() = default;
	TBox(const tvec &lower, const tvec &upper) : l(lower), u(upper) { }
	explicit TBox(int flag)
	{
		for (int index = 0; index < D; ++index)
		{
			l[index] = std::numeric_limits<T>::max();
			u[index] = std::numeric_limits<T>::lowest();
		}
	}

	TBox &operator=(const TBox &rhs)
	{
		for (int i = 0; i < D; ++i)
		{
			l[i] = rhs.l[i];
			u[i] = rhs.u[i];
		}
		return *this;
	}

	void set(const tvec &lower, const tvec &upper)
	{
		l = lower;
		u = upper;
	}

	void set(const T lower[D], const T upper[D])
	{
		for (int i = 0; i < D; ++i)
		{
			l[i] = lower[i];
			u[i] = upper[i];
		}
	}

	TBox combine(const TBox &b) const { return {glm::min(l, b.l), glm::max(u, b.u)}; }

	// test if the box can contain an another box
	bool contains(const TBox &b) const
	{
		// allow overlaps
		for (int i = 0; i < D; ++i)
			if (l[i] > b.l[i] || u[i] < b.u[i]) return false;
		return true;
	}

	// test if the box can contain a point
	bool contains(const tvec &p) const
	{
		// allow the case where the point is on the boundary
		for (int i = 0; i < D; ++i)
			if (l[i] < p[i] || u[i] > p[i]) return false;
		return true;
	}

	// test whether two aabb boxes overlaps
	bool overlaps(const TBox &b) const
	{
		for (int i = 0; i < D; ++i)
			if (l[i] > b.u[i] || b.l[i] > u[i]) return false;
		return true;
	}

	void setLower(const tvec &lowerBound) { l = lowerBound; }

	void setUpper(const tvec &upperBound) { u = upperBound; }

	const tvec &getLower() const { return l; }

	const tvec &getUpper() const { return u; }

	void setCenter(const tvec &center)
	{
		tvec ex = extents();
		setLower(center - ex);
		setUpper(center + ex);
	}

	void incCenter(const tvec &increment)
	{
		tvec c = center() + increment;
		setCenter(c);
	}

	void setHalfDims(const tvec &dim)
	{
		tvec c = center();
		setLower(c - dim);
		setUpper(c + dim);
	}

	void setDims(const glm::vec2 &dim) { setHalfDims(dim * 0.5f); }

	// expand the bounding box to contain a point.
	void extend(tvec p)
	{
		for (int i = 0; i < D; ++i)
		{
			if (p[i] < l[i]) l[i] = p[i];
			else if (p[i] > u[i]) u[i] = p[i];
		}
	}

	// expand the box to contain an another box
	void extend(const TBox &other)
	{
		for (int i = 0; i < D; ++i)
		{
			if (other.l[i] < l[i]) l[i] = other.l[i];
			if (other.u[i] > u[i]) u[i] = other.u[i];
		}
	}

	// get extended box and leave everything untouched
	TBox extended(const TBox &other) const
	{
		TBox res;
		for (int i = 0; i < D; ++i)
		{
			res.l[i] = l[i] < other.l[i] ? l[i] : other.l[i];
			res.u[i] = u[i] > other.u[i] ? u[i] : other.u[i];
		}
		return res;
	}

	void translate(const tvec &t)
	{
		for (int i = 0; i < D; ++i)
		{
			l[i] += t[i];
			u[i] += t[i];
		}
	}

	T perimeter() const
	{
		T res = 0;
		for (int i = 0; i < D; ++i) res += u[i] - l[i];
		return res * 2;
	}

	tvec extents() const
	{
		tvec res;
		for (int i = 0; i < D; ++i) res[i] = static_cast<T>(0.5f * (float)(u[i] - l[i]));
		return res;
	}

	tvec dimensions() const
	{
		tvec res;
		for (int i = 0; i < D; ++i) res[i] = u[i] - l[i];
		return res;
	}

	template <typename RealType>
	bool intersectsRay(
	    const glm::vec<D, RealType> &rayOrigin,
	    const glm::vec<D, RealType> &rayDirection) const
	{
		RealType tMin = -std::numeric_limits<RealType>::max();
		RealType tMax = std::numeric_limits<RealType>::max();
		for (int index = 0; index < D; ++index)
		{
			// check if parallel
			if (rayDirection[index] == (RealType)0)
			{
				if (rayOrigin[index] < l[index] || rayOrigin[index] > u[index]) return false;
				continue;
			}

			const RealType invDir = (RealType)1.0f / rayDirection[index];
			RealType deltaMin = (l[index] - rayOrigin[index]) * invDir;
			RealType deltaMax = (u[index] - rayOrigin[index]) * invDir;

			if (deltaMin > deltaMax) std::swap(deltaMin, deltaMax);

			if ((tMin > deltaMax) || (deltaMin > tMax)) return false;

			if (deltaMin > tMin) tMin = deltaMin;
			if (deltaMax < tMax) tMax = deltaMax;
		}
		return true;
	}

	bool overlaps(const tvec &center, T radius) const
	{
		T point[D];
		for (int index = 0; index < D; ++index) point[index] = center[index];

		// find the closest point near the circle
		for (int index = 0; index < D; ++index)
		{
			if (point[index] > u[index]) point[index] = u[index];
			if (point[index] < l[index]) point[index] = l[index];
		}

		// compute euclidean distance between points
		double distance = 0;
		for (int i = 0; i < D; ++i)
		{
			const T d = point[i] - center[i];
			distance += (double)d * d;
		}
		return distance < (radius * radius); // avoid square root
	}

	tvec center() const
	{
		tvec res;
		for (int i = 0; i < D; ++i) res[i] = l[i] + (u[i] - l[i]) / 2;
		return res;
	}

	T distanceSquare(const tvec &point) const
	{
		T d = Max(Max(l[0] - point[0], (T)0), point[0] - u[0]);
		d *= d;
		for (int i = 1; i < D; i++)
		{
			T temp = Max(Max(l[i] - point[i], (T)0), point[i] - u[i]);
			d += temp * temp;
		}
		return d;
	}

	float distance(const tvec &point) const { return Sqrt(float(distanceSquare(point))); }

	template <int VolumeMode, typename RealType> RealType volume() const
	{
		if (VolumeMode == detail::eSphericalVolume) return sphericalVolume<RealType>();
		return normalVolume<RealType>();
	}

	TBox quad2d(detail::RegionType type) const
	{
		const T halfW = (u[0] - l[0]) / 2;
		const T halfH = (u[1] - l[1]) / 2;
		switch (type)
		{
			case detail::eNW: {
				const tvec hmin = {l[0], l[1] + halfH};
				const tvec hmax = {l[0] + halfW, u[1]};
				return TBox(hmin, hmax);
			}
			case detail::eNE: {
				const tvec hmin = {l[0] + halfW, l[1] + halfH};
				return TBox(hmin, u);
			}
			case detail::eSW: {
				const tvec hmax = {l[0] + halfW, l[1] + halfH};
				return TBox(l, hmax);
			}
			case detail::eSE: {
				const tvec hmin = {l[0] + halfW, l[1]};
				const tvec hmax = {u[0], l[1] + halfH};
				return TBox(hmin, hmax);
			}
			default: assert(false); return *this;
		}
	}

	void valid() const
	{
		for (int i = 0; i < D; ++i) assert(l[i] <= u[i]);
	}

	template <typename RealType> RealType normalVolume() const
	{
		auto volume = static_cast<RealType>(1);
		for (int index = 0; index < D; ++index) volume *= u[index] - l[index];
		assert(volume >= (RealType)0);
		return volume;
	}

	// The exact volume of the bounding sphere for the given bbox
	template <typename RealType> RealType sphericalVolume() const
	{
		RealType sumOfSquares = 0;
		for (int i = 0; i < D; ++i)
		{
			RealType halfExtent = (u[i] - l[i]) * (RealType)0.5f;
			sumOfSquares += halfExtent * halfExtent;
		}

		auto radius = (RealType)Sqrt(sumOfSquares);

		// Pow maybe slow, so test for common dims like 2,3 and just use x*x, x*x*x.
		if (D == 3) { return (radius * radius * radius * unitSphereVolume<RealType>()); }
		else if (D == 2) { return (radius * radius * unitSphereVolume<RealType>()); }
		else { return (RealType)(Pow(radius, D) * unitSphereVolume<RealType>()); }
	}

	template <typename RealType> RealType unitSphereVolume() const
	{
		// Precomputed volumes of the unit spheres for the first few dimensions
		static const float kVolumes[] = {
		    0.000000f, 2.000000f, 3.141593f, // Dimension  0,1,2
		    4.188790f, 4.934802f, 5.263789f, // Dimension  3,4,5
		    5.167713f, 4.724766f, 4.058712f, // Dimension  6,7,8
		    3.298509f, 2.550164f, 1.884104f, // Dimension  9,10,11
		    1.335263f, 0.910629f, 0.599265f, // Dimension  12,13,14
		    0.381443f, 0.235331f, 0.140981f, // Dimension  15,16,17
		    0.082146f, 0.046622f, 0.025807f, // Dimension  18,19,20
		};
		static const auto val = (RealType)kVolumes[D];
		return val;
	}

	TBox getRotated(float rad) const
	{
		assert(D == 2);

		// get new dimensions
		float c = Cos(rad);
		float s = Sin(rad);
		glm::vec2 extent = extents();
		glm::vec2 newExtent(extent.y * s + extent.x * c, extent.x * s + extent.y * c);
		glm::vec2 ce = center();

		return {SubV(ce, newExtent), AddV(ce, newExtent)};
	}
};

using Box32 = TBox<std::int32_t>;
using Boxu32 = TBox<std::uint32_t>;
using Box64 = TBox<std::int64_t>;
using Box = TBox<float>;

template <typename T>
std::ostream &
operator<<(std::ostream &stream, const TBox<T, 2> &bbox)
{
	stream << "min: " << bbox.l[0] << " " << bbox.l[1] << " ";
	stream << "max: " << bbox.u[0] << " " << bbox.u[1];
	return stream;
}

template <typename T>
std::ostream &
operator<<(std::ostream &stream, const TBox<T, 3> &bbox)
{
	stream << "min: " << bbox.l[0] << " " << bbox.l[1] << " " << bbox.l[2] << " ";
	stream << "max: " << bbox.u[0] << " " << bbox.u[1] << " " << bbox.u[2] << " ";
	return stream;
}

//! @}
// NOLINTEND(modernize-use-nodiscard)
} // namespace ie

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // IE_AABB_HPP
