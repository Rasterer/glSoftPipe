#pragma once

#include "os.h"

#if defined(__clang__)
#	if defined(__x86_64__) || defined(__i386__)
#		include <x86intrin.h>
#	elif defined(__ARM_NEON__)
#		include <arm_neon.h>
#	endif

# define LIKELY(x)      __builtin_expect(!!(x), 1)
# define UNLIKELY(x)    __builtin_expect(!!(x), 0)
# define THREAD_LOCAL   __declspec(thread)

inline
unsigned char _BitScanForward(unsigned int *Index, unsigned Mask)
{
	*Index = __builtin_ctz(Mask);
	return (Mask != 0);
}

inline
unsigned char _BitScanReverse(unsigned int *Index, unsigned Mask)
{
	*Index = __builtin_clz(Mask);
	return (Mask != 0);
}

#elif defined(__INTEL_COMPILER)
# include <x86intrin.h>

# define LIKELY(x)      __builtin_expect(!!(x), 1)
# define UNLIKELY(x)    __builtin_expect(!!(x), 0)
# define THREAD_LOCAL   __thread


#elif defined(__GNUC__) || defined(__GNUG__)
#	if defined(__x86_64__) || defined(__i386__)
#		include <x86intrin.h>
#	elif defined(__ARM_NEON__)
#		include <arm_neon.h>
#	endif

# define LIKELY(x)      __builtin_expect(!!(x), 1)
# define UNLIKELY(x)    __builtin_expect(!!(x), 0)
# define THREAD_LOCAL   __thread
# define ALIGN(a)       __attribute__((aligned(a)))

inline
unsigned char _BitScanForward(unsigned int *Index, unsigned Mask)
{
	*Index = __builtin_ctz(Mask);
	return (Mask != 0);
}

inline
unsigned char _BitScanReverse(unsigned int *Index, unsigned Mask)
{
	*Index = __builtin_clz(Mask);
	return (Mask != 0);
}

#elif defined(_MSC_VER)
# include <intrin.h>

# define LIKELY(x)      (x)
# define UNLIKELY(x)    (x)
# define THREAD_LOCAL   __declspec(thread)

#endif
