#pragma once
#include <math.h>
#include "xrCore/math_constants.h"
#include "xrCommon/inlining_macros.h"
#include "xrCore/_bitwise.h" // iFloor

inline float _abs(float x) noexcept { return fabsf(x); }
inline float _sqrt(float x) noexcept { return sqrtf(x); }
inline float _sin(float x) noexcept { return sinf(x); }
inline float _cos(float x) noexcept { return cosf(x); }
inline double _abs(double x) noexcept { return fabs(x); }
inline double _sqrt(double x) noexcept { return sqrt(x); }
inline double _sin(double x) noexcept { return sin(x); }
inline double _cos(double x) noexcept { return cos(x); }

// comparisions
inline bool fsimilar(float a, float b, float cmp = EPS) { return _abs(a-b)<cmp; }
inline bool dsimilar(double a, double b, double cmp = EPS) { return _abs(a-b)<cmp; }

inline bool fis_zero(float val, float cmp = EPS_S) noexcept { return _abs(val) < cmp; }
inline bool dis_zero(double val, double cmp = EPS_S) noexcept { return _abs(val) < cmp; }

IC bool fLess			(float x, float y, float eps = EPS)		{ return (x < y - eps); }
IC bool fLessOrEqual	(float x, float y, float eps = EPS)		{ return (x < y + eps); }
IC bool fMore			(float x, float y, float eps = EPS)		{ return (y + eps < x); }
IC bool fMoreOrEqual	(float x, float y, float eps = EPS)		{ return (y - eps < x); }
IC bool fEqual			(float x, float y, float eps = EPS)		{ return (_abs(x - y) < eps); }
IC bool fIsZero			(float x, float eps = EPS)				{ return (_abs(x) < eps); }

// degree to radians and vice-versa
constexpr float  deg2rad(float  val) noexcept { return val*M_PI / 180; }
constexpr double deg2rad(double val) noexcept { return val*M_PI / 180; }
constexpr float  rad2deg(float  val) noexcept { return val*180 / M_PI; }
constexpr double rad2deg(double val) noexcept { return val*180 / M_PI; }

constexpr float deg2rad_half_mul = .5f * PI / 180.f;
constexpr float rad2deg_half_mul = 2.f * 180.f / PI;

ICF float deg2radHalf(float val) { return val * deg2rad_half_mul; }
ICF float rad2degHalf(float val) { return val * rad2deg_half_mul; }

// clamping/snapping
template <class T>
constexpr void clamp(T& val, const T& _low, const T& _high)
{
	if (val<_low)
		val = _low;
	else if (val>_high)
		val = _high;
}

// XXX: Check usages and provide overloads for native types where arguments are NOT references.
template <class T>
constexpr T clampr(const T& val, const T& _low, const T& _high)
{
	if (val < _low)
		return _low;
	if (val > _high)
		return _high;
	return val;
}

inline float snapto(float value, float snap)
{
	if (snap <= 0.f)
		return value;
	return float(iFloor((value + (snap*0.5f)) / snap)) * snap;
}
