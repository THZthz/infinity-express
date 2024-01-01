#ifndef __CANDYBOX_LINEAR_HPP__
#define __CANDYBOX_LINEAR_HPP__

#include <cstdint>
#include <cmath>
#include <cfloat>
#include <glm/vec2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/common.hpp>
#include <candybox/assert.hpp>

namespace candybox {
namespace m {

static constexpr float pi = 3.1415926535897932384626f;

using std::sqrt;
using std::pow;
using std::modf;
using std::fmod;
using std::sin;
using std::cos;
using std::tan;
using std::atan;
using std::atan2;
using std::acos;
using std::asin;
using std::hypot;
using std::ceil;
using std::floor;
using std::round;
using std::abs;

// clang-format off

static inline float pow2(float a) { return a * a; }

template <typename T> static inline constexpr T min(const T &a, const T &b) { return a < b ? a : b; }
template <typename T> static inline T min(const T &a, const T &b, const T &c) { return m::min(m::min(a, b), c); }
template <typename T> static inline T min(const T &a, const T &b, const T &c, const T &d) { return m::min(m::min(a, b, c), d); }
template <typename T> static inline constexpr T max(const T &a, const T &b) { return a > b ? a : b; }
template <typename T> static inline T max(const T &a, const T &b, const T &c) { return m::max(m::max(a, b), c); }
template <typename T> static inline T max(const T &a, const T &b, const T &c, const T &d) { return m::max(m::max(a, b, c), d); }
template <typename T> static inline T clamp(const T &a, const T &mn, const T &mx) { return glm::clamp(a, mn, mx); }
template <typename T> static inline T remap(T value, T inputStart, T inputEnd, T outputStart, T outputEnd) { return (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart; }

template <typename T> static inline T abs(T a) { return glm::abs(a); }
static inline float sign(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
static inline float cross2(float dx0, float dy0, float dx1, float dy1) { return dx1 * dy0 - dx0 * dy1; }

static inline bool approx_eq(float a, float b, float epsilon = 1e-6f) { return m::abs(a - b) <= epsilon; }
static inline bool approx_eq(const glm::vec2 &a, const glm::vec2 &b, float epsilon = 1e-6f) { return approx_eq(a.x, b.x, epsilon) && approx_eq(a.y, b.y, epsilon); }
template <typename T> static inline bool fuzzy_eq_esp(const T &a, const T &b, T epsilon = std::numeric_limits<T>::epsilon()) { return m::abs(a - b) < epsilon; }
template <typename T> static inline bool zero_esp(const T &a, T epsilon = std::numeric_limits<T>::epsilon()) { return m::abs(a) < epsilon; }
template <typename T> static inline bool fuzzy_eq(const T &a, const T &b) { return fuzzy_eq_esp(a, b); }
template <typename T> static inline bool fuzzy_zero(const T &x) { return fuzzy_eq_esp(x, static_cast<T>(0)); }
template <typename T> static inline bool fuzzy_neq(const T &a, const T &b) { return !fuzzy_eq(a, b); }
template <typename T> static inline bool fuzzy_not_zero(const T &x) { return !fuzzy_zero(x); }

template <typename T> static inline T subv(const T &a, const T &b) { return {a.x - b.x, a.y - b.y}; }
template <typename T> static inline T addv(const T &a, const T &b) { return {a.x + b.x, a.y + b.y}; }
template <typename T> static inline T negv(const T &a) { return {-a.x, -a.y}; }
template <typename T> static inline T lerpv(const T &a, const T &b, float t) { return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)}; }
static inline glm::vec2 muladdv(const glm::vec2 &a, float s, const glm::vec2 &b) { /*a + s * b*/return {a.x + s * b.x, a.y + s * b.y}; }
static inline glm::vec2 mulsubv(const glm::vec2 &a, float s, const glm::vec2 &b) { /*a - s * b*/return {a.x - s * b.x, a.y - s * b.y}; }
static inline glm::vec2 mulsv(float s, const glm::vec2 &v) { return {s * v.x, s * v.y}; }
static inline float lenv(const glm::vec2 &v) { return m::sqrt(v.x * v.x + v.y * v.y); }
static inline glm::vec2 lenv(const glm::vec2 &v, float length) { return mulsv(length / lenv(v), v); }
static inline float distv(const glm::vec2 &a, const glm::vec2 &b) { return lenv(subv(a, b)); }
static inline float dotv(const glm::vec2 &a, const glm::vec2 &b) { return a.x * b.x + a.y * b.y; }
static inline float distsqv(const glm::vec2 &a, const glm::vec2 &b) { glm::vec2 c = subv(a, b); return dotv(c, c); }
static inline glm::vec2 normv(const glm::vec2 &v) { float length = lenv(v); if (length < FLT_EPSILON) return {0, 0}; float invLength = 1.0f / length; return {invLength * v.x, invLength * v.y}; }
static inline void normv(glm::vec2 *v) { float length = lenv(*v); if (length < FLT_EPSILON) *v = {0, 0}; float invLength = 1.0f / length; *v = {invLength * v->x, invLength * v->y}; }
static inline glm::vec2 normv(const glm::vec2 &v, float &l) { l = lenv(v); if (l < FLT_EPSILON) return {0, 0}; float invLength = 1.0f / l; return {invLength * v.x, invLength * v.y}; }
static inline glm::vec2 crosssv(float s, const glm::vec2 &v) { return {-s * v.y, s * v.x}; }
static inline glm::vec2 crossvs(const glm::vec2 &v, float s) { return {s * v.y, -s * v.x}; }
static inline float crossv(const glm::vec2 &a, const glm::vec2 &b) { return a.x * b.y - a.y * b.x; }
static inline glm::vec2 absv(const glm::vec2 &a) { return {abs(a.x), abs(a.y)}; }
static inline glm::vec2 minv(const glm::vec2 &a, const glm::vec2 &b) { return {min(a.x, b.x), min(a.y, b.y)}; }
static inline glm::vec2 maxv(const glm::vec2 &a, const glm::vec2 &b) { return {max(a.x, b.x), max(a.y, b.y)}; }
static inline glm::vec2 clampv(const glm::vec2 &a, const glm::vec2 &lo, glm::vec2 hi) { return maxv(lo, minv(a, hi)); }
static inline glm::vec2 scalev(const glm::vec2 &a, float b) { return {a.x * b, a.y * b}; }
static inline glm::vec2 divv(const glm::vec2 &a, float b) { return scalev(a, 1.0f / b); }
static inline glm::vec2 skewv(const glm::vec2 &a) { return {-a.y, a.x}; }
static inline glm::vec2 ccw90v(const glm::vec2 &a) { return {a.y, -a.x}; }
static inline float det2(const glm::vec2 &a, const glm::vec2 &b) { return a.x * b.y - a.y * b.x; }
static inline bool parallelv(const glm::vec2 &a, const glm::vec2 &b, float kTol) { float k = lenv(a) / lenv(b); glm::vec2 nb = scalev(b, k); if (abs(a.x - nb.x) < kTol && abs(a.y - nb.y) < kTol) return true; return false; }
static inline float projectv(const glm::vec2 &left, const glm::vec2 &right) { return dotv(left, right) / dotv(right, right); }
static inline float anglev(const glm::vec2 &a, const glm::vec2 &b) { return m::acos(dotv(a, b) / (lenv(a) * lenv(b))); }

static inline float DegToRad(float deg) { return deg / 180.0f * (float)m::pi; }
static inline double DegToRad(double deg) { return deg / 180.0 * m::pi; }
static inline float RadToDeg(float rad) { return rad / (float)m::pi * 180.0f; }
static inline double RadToDeg(double rad) { return rad / m::pi * 180.0; }

// clang-format on

/// Given parameters a,x,b,y returns the value (b*x+a*y)/(a+b), or (x+y)/2 if a==b==0.
///
/// It requires that a,b >= 0, and enforces this in the rare case that one argument is
/// slightly negative.
///
/// The implementation is extremely stable numerically.
/// In particular it guarantees that the result r satisfies MIN(x,y) <= r <= MAX(x,y),
/// and the results are very accurate even when a and b differ greatly in magnitude.
template <typename T> static inline T interpolate(T &a, T &x, T &b, T &y) {
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
static inline T interpolate(const T A, const T B, const V t) {
  candybox_assert(t >= 0);
  candybox_assert(t <= 1);
  return A + ((B - A) * t);
}

static inline float fisr(float x, int iterations) {
  union {
    float f;
    uint32_t i;
  } conv;
  conv.f = x;
  conv.i = 0x5f3759df - (conv.i >> 1);
  for (int i = 0; i < iterations; i++) conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
  return conv.f;
}

} // namespace m
} // namespace candybox
#endif // __CANDYBOX_LINEAR_HPP__
