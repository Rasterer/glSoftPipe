#pragma once

#include "compiler.h"

#define EQUAL(a, b) (abs((a) - (b)) <= FLT_EPSILON)

#define SWAP(a, b, tmp) {	\
	tmp = a;				\
	a = b;					\
	b = tmp;				\
}

#define DIFFERENT_SIGNS(a, b)	((a) >= 0.0f && (b) < 0.0f || (a) < 0.0f && (b) >=0.0f)

// b should be power of 2.
// Watch out for 32 & 64 mix and match!
#define ROUND_UP(a, b)       (((a) + ((b) - 1)) & (~((b) - 1)))
#define ROUND_DOWN(a, b)  ((a) & (~((b) - 1)))

#define ROUND_UP_SIMD(a, b) \
	(_mm_and_si128(_mm_sub_epi32(_mm_add_epi32((a), (b)), _mm_set1_epi32(1)),	\
	_mm_andnot_si128(_mm_sub_epi32((b), _mm_set1_epi32(1)), _mm_set1_epi32(0xFFFFFFFF))))

#define ROUND_DOWN_SIMD(a, b)	\
	(_mm_and_si128(a, _mm_andnot_si128(_mm_sub_epi32((b), _mm_set1_epi32(1)), _mm_set1_epi32(0xFFFFFFFF))))

// *.4 fixed point precision
#define FIXED_POINT4          16
#define FIXED_POINT4_SHIFT    4

#define FIXED_POINT8          256
#define FIXED_POINT8_SHIFT    8

#define FIXED_POINT16         65536
#define FIXED_POINT16_SHIFT   16

namespace glsp {

template <int N>
static inline int fixedpoint_cast(float fp)
{
	return (int)(fp * N + 0.5f);
}

template <typename T>
static inline T clamp(T val, T l, T h)
{
	return (val < l) ? l: (val > h) ? h: val;
}

static inline __m128 _simd_clamp_ps(__m128 val, __m128 min, __m128 max)
{
	return _mm_min_ps(_mm_max_ps(val, min), max);
}

static inline __m128i _simd_clamp_epi32(__m128i val, __m128i min, __m128i max)
{
	return _mm_min_epi32(_mm_max_epi32(val, min), max);
}

static inline __m128i MAWrapper(const __m128i &v, const __m128i &stride, const __m128i &h)
{
#if defined(__AVX2__)
	// FMA, relaxed floating-point precision
	return _mm_fmadd_epi32(v, stride, h);
#else
	return _mm_add_epi32(_mm_mullo_epi32(v, stride), h);
#endif
}

static inline __m128 MAWrapper(const __m128 &v, const __m128 &stride, const __m128 &h)
{
#if defined(__AVX2__)
	// FMA, relaxed floating-point precision
	return _mm_fmadd_ps(v, stride, h);
#else
	return _mm_add_ps(_mm_mul_ps(v, stride), h);
#endif
}

static inline __m128 length(__m128 &vX, __m128 &vY, __m128 &vZ)
{
	__m128 vSquareX = _mm_mul_ps(vX, vX);
	__m128 vSquareY = _mm_mul_ps(vY, vY);
	__m128 vSquareZ = _mm_mul_ps(vZ, vZ);
	return _mm_sqrt_ps(_mm_add_ps(_mm_add_ps(vSquareX, vSquareY), vSquareZ));
}

static inline void normalize(__m128 &vX, __m128 &vY, __m128 &vZ)
{
	__m128 vLengthReciprocal = _mm_rcp_ps(length(vX, vY, vZ));
	vX = _mm_mul_ps(vX, vLengthReciprocal);
	vY = _mm_mul_ps(vY, vLengthReciprocal);
	vZ = _mm_mul_ps(vZ, vLengthReciprocal);
}

static inline __m128 dot(__m128 &vX0, __m128 &vY0, __m128 &vZ0, __m128 &vX1, __m128 &vY1, __m128 &vZ1)
{
	__m128 vRes = _mm_mul_ps(vX0, vX1);
	vRes = MAWrapper(vY0, vY1, vRes);
	vRes = MAWrapper(vZ0, vZ1, vRes);
	return vRes;
}

static inline void reflect(__m128 &vIncidentX, __m128 &vIncidentY, __m128 &vIncidentZ,
			__m128 &vNormalX,   __m128 &vNormalY,   __m128 &vNormalZ,
			__m128 &vReflectX,  __m128 &vReflectY,  __m128 &vReflectZ)
{
	__m128 vDotProduct = dot(vNormalX, vNormalY, vNormalZ, vIncidentX, vIncidentY, vIncidentZ);
	vReflectX = _mm_sub_ps(vIncidentX, _mm_mul_ps(_mm_mul_ps(vDotProduct, vNormalX), _mm_set_ps1(2.0f)));
	vReflectY = _mm_sub_ps(vIncidentY, _mm_mul_ps(_mm_mul_ps(vDotProduct, vNormalY), _mm_set_ps1(2.0f)));
	vReflectZ = _mm_sub_ps(vIncidentZ, _mm_mul_ps(_mm_mul_ps(vDotProduct, vNormalZ), _mm_set_ps1(2.0f)));
}

} // namespace glsp
