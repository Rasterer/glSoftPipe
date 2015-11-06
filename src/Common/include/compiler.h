#pragma once

#include "os.h"

#if defined(__clang__)
# define LIKELY(x)      __builtin_expect(!!(x), 1)
# define UNLIKELY(x)    __builtin_expect(!!(x), 0)

# define THREAD_LOCAL   __declspec(thread)

#elif defined(__INTEL_COMPILER)
# define LIKELY(x)      __builtin_expect(!!(x), 1)
# define UNLIKELY(x)    __builtin_expect(!!(x), 0)

# define THREAD_LOCAL   __thread

#elif defined(__GNUC__) || defined(__GNUG__)
# define LIKELY(x)      __builtin_expect(!!(x), 1)
# define UNLIKELY(x)    __builtin_expect(!!(x), 0)

# define THREAD_LOCAL   __thread

#elif defined(_MSC_VER)
# define LIKELY(x)      (x)
# define UNLIKELY(x)    (x)

# define THREAD_LOCAL   __declspec(thread)

#endif
