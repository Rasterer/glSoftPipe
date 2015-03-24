#define EQUAL(a, b) (abs((a) - (b)) <= FLT_EPSILON)

#define SWAP(a, b, tmp) {	\
	tmp = a;				\
	a = b;					\
	b = tmp;				\
}

#define DIFFERENT_SIGNS(a, b)	((a) >= 0.0f && (b) < 0.0f || (a) < 0.0f && (b) >=0.0f)

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))
