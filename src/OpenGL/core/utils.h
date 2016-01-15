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
inline int fixedpoint_cast(float fp)
{
	return (int)(fp * N + 0.5f);
}

template <typename T>
inline T clamp(T val, T l, T h)
{
	return (val < l) ? l: (val > h) ? h: val;
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

} // namespace glsp
