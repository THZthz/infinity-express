#ifndef IE_EASING_HPP
#define IE_EASING_HPP

#include <cmath>
#include "utils/Linear.hpp"

//███████╗ █████╗ ███████╗██╗███╗   ██╗ ██████╗
//██╔════╝██╔══██╗██╔════╝██║████╗  ██║██╔════╝
//█████╗  ███████║███████╗██║██╔██╗ ██║██║  ███╗
//██╔══╝  ██╔══██║╚════██║██║██║╚██╗██║██║   ██║
//███████╗██║  ██║███████║██║██║ ╚████║╚██████╔╝
//╚══════╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝

namespace ie {
namespace easing {


//! \defgroup Easing
//! t: current time
//! b: start value
//! c: change in value
//! d: duration
//! @{

enum class Type
{
	kLinear = 0,
	kInQuad,
	kOutQuad,
	kInOutQuad,
	kInCubic,
	kOutCubic,
	kInOutCubic,
	kInQuart,
	kOutQuart,
	kInOutQuart,
	kInQuint,
	kOutQuint,
	kInOutQuint,
	kInSine,
	kOutSine,
	kInOutSine,
	kInExpo,
	kOutExpo,
	kInOutExpo,
	kInCirc,
	kOutCirc,
	kInOutCirc,
	kInBack,
	kOutBack,
	kInOutBack,
	kInElastic,
	kOutElastic,
	kInOutElastic,
	kInBounce,
	kOutBounce,
	kInOutBounce,
	kNums
};

typedef double (*Func)(double t, double b, double c, double d);

/// Simple linear tween - no easing, no acceleration.
static inline double
Linear(double t, double b, double c, double d)
{
	return c * t / d + b;
}

/// Quadratic easing in - accelerating from zero velocity.
static inline double
InQuad(double t, double b, double c, double d)
{
	t /= d;
	return c * t * t + b;
}

/// Quadratic easing out - decelerating to zero velocity.
static inline double
OutQuad(double t, double b, double c, double d)
{
	t /= d;
	return -c * t * (t - 2) + b;
}

/// Quadratic easing in/out - acceleration until halfway, then deceleration.
static inline double
InOutQuad(double t, double b, double c, double d)
{
	t /= d / 2;
	if (t < 1) return c / 2 * t * t + b;
	t--;
	return -c / 2 * (t * (t - 2) - 1) + b;
}

/// Cubic easing in - accelerating from zero velocity.
static inline double
InCubic(double t, double b, double c, double d)
{
	t /= d;
	return c * t * t * t + b;
}

/// Cubic easing out - decelerating to zero velocity.
static inline double
OutCubic(double t, double b, double c, double d)
{
	t /= d;
	t--;
	return c * (t * t * t + 1) + b;
}

/// Cubic easing in/out - acceleration until halfway, then deceleration.
static inline double
InOutCubic(double t, double b, double c, double d)
{
	t /= d / 2;
	if (t < 1) return c / 2 * t * t * t + b;
	t -= 2;
	return c / 2 * (t * t * t + 2) + b;
}

/// Quartic easing in - accelerating from zero velocity.
static inline double
InQuart(double t, double b, double c, double d)
{
	t /= d;
	return c * t * t * t * t + b;
}

/// Quartic easing out - decelerating to zero velocity.
static inline double
OutQuart(double t, double b, double c, double d)
{
	t /= d;
	t--;
	return -c * (t * t * t * t - 1) + b;
}

/// Quartic easing in/out - acceleration until halfway, then deceleration.
static inline double
InOutQuart(double t, double b, double c, double d)
{
	t /= d / 2;
	if (t < 1) return c / 2 * t * t * t * t + b;
	t -= 2;
	return -c / 2 * (t * t * t * t - 2) + b;
}

/// Quintic easing in - accelerating from zero velocity.
static inline double
InQuint(double t, double b, double c, double d)
{
	t /= d;
	return c * t * t * t * t * t + b;
}

/// Quintic easing out - decelerating to zero velocity.
static inline double
OutQuint(double t, double b, double c, double d)
{
	t /= d;
	t--;
	return c * (t * t * t * t * t + 1) + b;
}

/// Quintic easing in/out - acceleration until halfway, then deceleration.
static inline double
InOutQuint(double t, double b, double c, double d)
{
	t /= d / 2;
	if (t < 1) return c / 2 * t * t * t * t * t + b;
	t -= 2;
	return c / 2 * (t * t * t * t * t + 2) + b;
}

/// Sinusoidal easing in - accelerating from zero velocity.
static inline double
InSine(double t, double b, double c, double d)
{
	return -c * std::cos(t / d * (PI / 2)) + c + b;
}

/// Sinusoidal easing out - decelerating to zero velocity.
static inline double
OutSine(double t, double b, double c, double d)
{
	return c * std::sin(t / d * (PI / 2)) + b;
}

/// Sinusoidal easing in/out - accelerating until halfway, then decelerating.
static inline double
InOutSine(double t, double b, double c, double d)
{
	return -c / 2 * (std::cos(PI * t / d) - 1) + b;
}

/// Exponential easing in - accelerating from zero velocity.
static inline double
InExpo(double t, double b, double c, double d)
{
	return c * std::pow(2, 10 * (t / d - 1)) + b;
}

/// Exponential easing out - decelerating to zero velocity.
static inline double
OutExpo(double t, double b, double c, double d)
{
	return c * (-std::pow(2, -10 * t / d) + 1) + b;
}

/// Exponential easing in/out - accelerating until halfway, then decelerating.
static inline double
InOutExpo(double t, double b, double c, double d)
{
	t /= d / 2;
	if (t < 1) return c / 2 * std::pow(2, 10 * (t - 1)) + b;
	t--;
	return c / 2 * (-std::pow(2, -10 * t) + 2) + b;
}

/// Circular easing in - accelerating from zero velocity.
static inline double
InCirc(double t, double b, double c, double d)
{
	t /= d;
	return -c * (std::sqrt(1 - t * t) - 1) + b;
}

/// Circular easing out - decelerating to zero velocity.
static inline double
OutCirc(double t, double b, double c, double d)
{
	t /= d;
	t--;
	return c * std::sqrt(1 - t * t) + b;
}

/// Circular easing in / out - acceleration until halfway, then deceleration.
static inline double
InOutCirc(double t, double b, double c, double d)
{
	t /= d / 2;
	if (t < 1) return -c / 2 * (std::sqrt(1 - t * t) - 1) + b;
	t -= 2;
	return c / 2 * (std::sqrt(1 - t * t) + 1) + b;
}

static inline double
InBack(double t, double b, double c, double d)
{
	double o = 1.70158f;
	double z = ((o + 1.0f) * t) - o;
	double t0 = t / d;
	return (t0 * t0 * z) * c + b;
}

static inline double
OutBack(double t, double b, double c, double d)
{
	double o, z, n;
	o = 1.70158f;
	n = t / d - 1.0f;
	z = (o + 1.0f) * n + o;
	return (n * n * z + 1.0f) * c + b;
}

static inline double
InOutBack(double t, double b, double c, double d)
{
	double o, z, n, m, s, x;

	o = 1.70158f;
	s = o * 1.525f;
	x = 0.5;
	n = (t / d) / 0.5f;

	if (n < 1.0f)
	{
		z = (s + 1) * n - s;
		m = n * n * z;
		return x * m;
	}

	n -= 2.0f;
	z = (s + 1.0f) * n + s;
	m = (n * n * z) + 2;

	return x * m * c + b;
}

static inline double
InElastic(double t, double b, double c, double d)
{
	double t0 = t / d;
	double f = std::sin(13.0f * 2.f * PI * t0) * std::pow(2, 10 * (t0 - 1));
	return b + c * f;
}

static inline double
OutElastic(double t, double b, double c, double d)
{
	double t0 = t / d;
	double f = std::sin(-13.0f * 2.f * PI * (t0 + 1.0f)) * std::pow(2, -10 * t0) + 1.0f;
	return b + c * f;
}

static inline double
InOutElastic(double t, double b, double c, double d)
{
	double t0 = t / d;
	double a = 2.0f * t0;
	if (t0 < 0.5f) return 0.5f * std::sin(13.0f * 2.f * PI * a) * std::pow(2, 10 * (a - 1));
	double f = 0.5f * (std::sin(-13.0f * 2.f * PI * a) * std::pow(2, -10 * (a - 1)) + 2);
	return f * c + b;
}

namespace priv {
static inline double
_OutBounce(double t)
{
	double tt;

	tt = t * t;

	if (t < (4.0f / 11.0f))
	{
		return (121.0f * tt) / 16.0f;
	}
	else if (t < 8.0f / 11.0f)
	{
		return ((363.0f / 40.0f) * tt) - ((99.0f / 10.0f) * t) + (17.0f / 5.0f);
	}
	else if (t < (9.0f / 10.0f))
	{
		return (4356.0f / 361.0f) * tt - (35442.0f / 1805.0f) * t + (16061.0f / 1805.0f);
	}
	else
	{
		return ((54.0f / 5.0f) * tt) - ((513.0f / 25.0f) * t) + (268.0f / 25.0f);
	}
}
}

static inline double
OutBounce(double t, double b, double c, double d)
{
	return priv::_OutBounce(t / d) * c + b;
}

static inline double
InBounce(double t, double b, double c, double d)
{
	return (1.0f - priv::_OutBounce(1.0f - t / d)) * c + b;
}

static inline double
InOutBounce(double t, double b, double c, double d)
{
	double t0 = t / d;
	double f;
	if (t < 0.5f) f = 0.5f * (1.0f - priv::_OutBounce(t0 * 2.0f));
	else f = 0.5f * priv::_OutBounce(t0 * 2.0f - 1.0f) + 0.5f;
	return f * c + b;
}

static inline Func
GetMappingFunc(Type type)
{
	Func functions[static_cast<int>(Type::kNums)] = {
	    Linear,    InQuad,    OutQuad,    InOutQuad, InCubic,   OutCubic,   InOutCubic,
	    InQuart,   OutQuart,  InOutQuart, InQuint,   OutQuint,  InOutQuint, InSine,
	    OutSine,   InOutSine, InExpo,     OutExpo,   InOutExpo, InCirc,     OutCirc,
	    InOutCirc, InBack,    OutBack,    InOutBack, InElastic, OutElastic, InOutElastic,
	    InBounce,  OutBounce, InOutBounce};

	return functions[static_cast<int>(type)];
}

//! @}

} // namespace easing
} // namespace ie

#endif // IE_EASING_HPP
