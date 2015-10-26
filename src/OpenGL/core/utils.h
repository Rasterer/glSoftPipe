#define EQUAL(a, b) (abs((a) - (b)) <= FLT_EPSILON)

#define SWAP(a, b, tmp) {	\
	tmp = a;				\
	a = b;					\
	b = tmp;				\
}

#define DIFFERENT_SIGNS(a, b)	((a) >= 0.0f && (b) < 0.0f || (a) < 0.0f && (b) >=0.0f)

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

// b should be power of 2.
// Watch out for 32 & 64 mix and match!
#define ALIGN(a, b)       (((a) + ((b) - 1)) & (~((b) - 1)))
#define ROUND_UP(a, b)    (ALIGN((a), (b)))
#define ROUND_DOWN(a, b)  ((a) & ~((b) - 1))

// *.4 fixed point precision
#define FIXED_POINT4          16
#define FIXED_POINT4_SHIFT    4

#define FIXED_POINT8          256
#define FIXED_POINT8_SHIFT    8

#define FIXED_POINT16         65536
#define FIXED_POINT16_SHIFT   16

template <int N>
inline int fixedpoint_cast<N>(float fp)
{
	return (int)(fp * N + 0.5f);
}
