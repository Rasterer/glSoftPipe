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
#define ALIGN(a, b)  (((a) + ((b) - 1)) & (~((b) - 1)))
