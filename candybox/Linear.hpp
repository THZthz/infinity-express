#ifndef IE_LINEAR_HPP
#define IE_LINEAR_HPP

#include <cstdint>
#include <cmath>
#include <cfloat>
#include <glm/vec2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/common.hpp>
#include "candybox/Macros.hpp"

namespace ie {

static constexpr float PI = 3.14159265358979323846f;

// clang-format off
static inline float Sqrt(float a) { return sqrtf(a); }
static inline double Sqrt(double a) { return sqrt(a); }
static inline float Pow(float a, float n) { return powf(a, n); }
static inline double Pow(double a, double n) { return pow(a, n); }
static inline float Mod(float a, float b) { return fmodf(a, b); }
static inline float Sin(float a) { return sinf(a); }
static inline double Sin(double a) { return sin(a); }
static inline float Cos(float a) { return cosf(a); }
static inline double Cos(double a) { return cos(a); }
static inline float Tan(float a) { return tanf(a); }
static inline float Atan2(float a, float b) { return atan2f(a, b); }
static inline float Acos(float a) { return acosf(a); }
static inline float Asin(float a) { return asinf(a); }
static inline float Pow2(float a) { return a * a; }
static inline float Hypot(float a, float b) { return Sqrt(Pow2(a) + Pow2(b)); }
static inline float Ceil(float x) { return ceilf(x); }
static inline float Floor(float x) { return floorf(x); }
static inline float Round(float a) { return roundf(a); }

template <typename T> static inline constexpr T Min(const T &a, const T &b) { return a < b ? a : b; }
template <typename T> static inline T Min(const T &a, const T &b, const T &c) { return Min(Min(a, b), c); }
template <typename T> static inline T Min(const T &a, const T &b, const T &c, const T &d) { return Min(Min(a, b, c), d); }
template <typename T> static inline constexpr T Max(const T &a, const T &b) { return a > b ? a : b; }
template <typename T> static inline T Max(const T &a, const T &b, const T &c) { return Max(Max(a, b), c); }
template <typename T> static inline T Max(const T &a, const T &b, const T &c, const T &d) { return Max(Max(a, b, c), d); }
template <typename T> static inline T Clamp(const T &a, const T &mn, const T &mx) { return glm::clamp(a, mn, mx); }
template <typename T> static inline T Remap(T value, T inputStart, T inputEnd, T outputStart, T outputEnd) { return (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart; }

template <typename T> static inline T Abs(T a) { return glm::abs(a); }
static inline float Sign(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
static inline float Cross2(float dx0, float dy0, float dx1, float dy1) { return dx1 * dy0 - dx0 * dy1; }

static inline bool ApproxEqual(float a, float b, float epsilon = 1e-6f) { return Abs(a - b) <= epsilon; }
static inline bool ApproxEqual(const glm::vec2 &a, const glm::vec2 &b, float epsilon = 1e-6f) { return ApproxEqual(a.x, b.x, epsilon) && ApproxEqual(a.y, b.y, epsilon); }
static inline bool IsEqualWithEpsilon(const float &a, const float &b, float epsilon = FLT_EPSILON) { return Abs(a - b) < epsilon; }
static inline bool IsEqualWithEpsilon(const double &a, const double &b, double epsilon = DBL_EPSILON) { return Abs(a - b) < epsilon; }
static inline bool IsZeroWithEpsilon(const float &a, float epsilon = FLT_EPSILON) { return Abs(a) < epsilon; }
static inline bool IsZeroWithEpsilon(const double &a, double epsilon = DBL_EPSILON) { return Abs(a) < epsilon; }
static inline bool FuzzyIsEqual(const float &a, const float &b) { return IsEqualWithEpsilon(a, b); }
static inline bool FuzzyIsEqual(const double &a, const double &b) { return IsEqualWithEpsilon(a, b); }
static inline bool FuzzyIsZero(const float &x) { return IsEqualWithEpsilon(x, 0.f); }
static inline bool FuzzyIsZero(const double &x) { return IsEqualWithEpsilon(x, 0.0); }
static inline bool FuzzyNotEqual(const float &a, const float &b) { return !FuzzyIsEqual(a, b); }
static inline bool FuzzyNotEqual(const double &a, const double &b) { return !FuzzyIsEqual(a, b); }
static inline bool FuzzyNotZero(const float &x) { return !FuzzyIsZero(x); }
static inline bool FuzzyNotZero(const double &x) { return !FuzzyIsZero(x); }

template <typename T> static inline T SubV(const T &a, const T &b) { return {a.x - b.x, a.y - b.y}; }
template <typename T> static inline T AddV(const T &a, const T &b) { return {a.x + b.x, a.y + b.y}; }
template <typename T> static inline T NegV(const T &a) { return {-a.x, -a.y}; }
template <typename T> static inline T LerpV(const T &a, const T &b, float t) { return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)}; }
static inline glm::vec2 MulAddV(const glm::vec2 &a, float s, const glm::vec2 &b) { /*a + s * b*/return {a.x + s * b.x, a.y + s * b.y}; }
static inline glm::vec2 MulSubV(const glm::vec2 &a, float s, const glm::vec2 &b) { /*a - s * b*/return {a.x - s * b.x, a.y - s * b.y}; }
static inline glm::vec2 MulSV(float s, const glm::vec2 &v) { return {s * v.x, s * v.y}; }
static inline float LenV(const glm::vec2 &v) { return Sqrt(v.x * v.x + v.y * v.y); }
static inline glm::vec2 LenV(const glm::vec2 &v, float length) { return MulSV(length / LenV(v), v); }
static inline float DistV(const glm::vec2 &a, const glm::vec2 &b) { return LenV(SubV(a, b)); }
static inline float DotV(const glm::vec2 &a, const glm::vec2 &b) { return a.x * b.x + a.y * b.y; }
static inline float DistSqV(const glm::vec2 &a, const glm::vec2 &b) { glm::vec2 c = SubV(a, b); return DotV(c, c); }
static inline glm::vec2 NormV(const glm::vec2 &v) { float length = LenV(v); if (length < FLT_EPSILON) return {0, 0}; float invLength = 1.0f / length; return {invLength * v.x, invLength * v.y}; }
static inline void NormV(glm::vec2 *v) { float length = LenV(*v); if (length < FLT_EPSILON) *v = {0, 0}; float invLength = 1.0f / length; *v = {invLength * v->x, invLength * v->y}; }
static inline glm::vec2 NormV(const glm::vec2 &v, float &l) { l = LenV(v); if (l < FLT_EPSILON) return {0, 0}; float invLength = 1.0f / l; return {invLength * v.x, invLength * v.y}; }
static inline glm::vec2 CrossSV(float s, const glm::vec2 &v) { return {-s * v.y, s * v.x}; }
static inline glm::vec2 CrossVS(const glm::vec2 &v, float s) { return {s * v.y, -s * v.x}; }
static inline float CrossV(const glm::vec2 &a, const glm::vec2 &b) { return a.x * b.y - a.y * b.x; }
static inline glm::vec2 AbsV(const glm::vec2 &a) { return {Abs(a.x), Abs(a.y)}; }
static inline glm::vec2 MinV(const glm::vec2 &a, const glm::vec2 &b) { return {Min(a.x, b.x), Min(a.y, b.y)}; }
static inline glm::vec2 MaxV(const glm::vec2 &a, const glm::vec2 &b) { return {Max(a.x, b.x), Max(a.y, b.y)}; }
static inline glm::vec2 ClampV(const glm::vec2 &a, const glm::vec2 &lo, glm::vec2 hi) { return MaxV(lo, MinV(a, hi)); }
static inline glm::vec2 ScaleV(const glm::vec2 &a, float b) { return {a.x * b, a.y * b}; }
static inline glm::vec2 DivV(const glm::vec2 &a, float b) { return ScaleV(a, 1.0f / b); }
static inline glm::vec2 SkewV(const glm::vec2 &a) { return {-a.y, a.x}; }
static inline glm::vec2 CCW90V(const glm::vec2 &a) { return {a.y, -a.x}; }
static inline float Det2(const glm::vec2 &a, const glm::vec2 &b) { return a.x * b.y - a.y * b.x; }
static inline bool ParallelV(const glm::vec2 &a, const glm::vec2 &b, float kTol) { float k = LenV(a) / LenV(b); glm::vec2 nb = ScaleV(b, k); if (Abs(a.x - nb.x) < kTol && Abs(a.y - nb.y) < kTol) return true; return false; }
static inline float ProjectV(const glm::vec2 &left, const glm::vec2 &right) { return DotV(left, right) / DotV(right, right); }
static inline float AngleV(const glm::vec2 &a, const glm::vec2 &b) { return Acos(DotV(a, b) / (LenV(a) * LenV(b))); }

static inline float DegToRad(float deg) { return deg / 180.0f * (float)PI; }
static inline double DegToRad(double deg) { return deg / 180.0 * PI; }
static inline float RadToDeg(float rad) { return rad / (float)PI * 180.0f; }
static inline double RadToDeg(double rad) { return rad / PI * 180.0; }

// clang-format on


/// Given parameters a,x,b,y returns the value (b*x+a*y)/(a+b), or (x+y)/2 if a==b==0.
///
/// It requires that a,b >= 0, and enforces this in the rare case that one argument is
/// slightly negative.
///
/// The implementation is extremely stable numerically.
/// In particular it guarantees that the result r satisfies MIN(x,y) <= r <= MAX(x,y),
/// and the results are very accurate even when a and b differ greatly in magnitude.
template <typename T>
static inline T
interpolate(T &a, T &x, T &b, T &y)
{
	a = (a < 0) ? 0 : a;
	b = (b < 0) ? 0 : b;
	if (a <= b) return b == 0 ? ((x + y) / 2) : (x + (y - x) * (a / (a + b)));
	return y + (x - y) * (b / (a + b));
}

/// Linearly interpolate between A and B.
/// If t is 0, returns A.
/// If t is 1, returns B.
/// If t is something else, returns value linearly interpolated between A and B.
template <typename T, typename V>
static inline T
interpolate(const T A, const T B, const V t)
{
	assert(t >= 0);
	assert(t <= 1);
	return A + ((B - A) * t);
}

static inline API float
fisr(float x, int iterations)
{
	union
	{
		float f;
		uint32_t i;
	} conv;
	conv.f = x;
	conv.i = 0x5f3759df - (conv.i >> 1);
	for (int i = 0; i < iterations; i++) conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
	return conv.f;
}

} // namespace ie
#endif // IE_LINEAR_HPP
