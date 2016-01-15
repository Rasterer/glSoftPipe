// Avoid MSVC to see min max as macros rather than std functions
#define NOMINMAX 1

#include "TBDR.h"

#include <algorithm>
#include <cstring>

#include "ThreadPool.h"
#include "MemoryPool.h"
#include "GLContext.h"
#include "DrawEngine.h"
#include "PixelBackend.h"
#include "glsp_spinlock.h"
#include "utils.h"
#include "compiler.h"


namespace glsp {

static std::vector<Triangle *> s_DispList         [MAX_TILES_IN_HEIGHT][MAX_TILES_IN_WIDTH];
static std::vector<Triangle *> s_FullCoverDispList[MAX_TILES_IN_HEIGHT][MAX_TILES_IN_WIDTH];

static ::glsp::SpinLock s_DispListLock;
static ::glsp::SpinLock s_FullCoverDispListLock;


Binning::Binning():
	PipeStage("Binning", DrawEngine::getDrawEngine())
{
}

Binning::~Binning()
{
}

void Binning::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	onBinning(bat);
}

void Binning::onBinning(Batch *bat)
{
	Primitive *prim[4];
	int i;

	auto iter = bat->mPrims.begin();

	while(true)
	{
		// Fetch four primitives in one go.
		for (i = 0; i < 4; ++i, ++iter)
		{
			if (iter != bat->mPrims.end())
			{
				prim[i] = *iter;
			}
			else
			{
				break;
			}
		}

		if (i != 4)
			break;

		Triangle *tri0 = new(MemoryPoolMT::get()) Triangle(*prim[0]);
		Triangle *tri1 = new(MemoryPoolMT::get()) Triangle(*prim[1]);
		Triangle *tri2 = new(MemoryPoolMT::get()) Triangle(*prim[2]);
		Triangle *tri3 = new(MemoryPoolMT::get()) Triangle(*prim[3]);

		SetupTriangleSIMD(tri0, tri1, tri2, tri3);

		CoarseRasterizing(tri0);
		CoarseRasterizing(tri1);
		CoarseRasterizing(tri2);
		CoarseRasterizing(tri3);
	}

	for (int j = 0; j < i; j++)
	{
		Triangle *tri = new(MemoryPoolMT::get()) Triangle(*prim[j]);
		SetupTriangle(tri);
		CoarseRasterizing(tri);
	}
}

void Binning::SetupTriangleSIMD(Triangle *tri0, Triangle *tri1, Triangle *tri2, Triangle *tri3)
{
	Primitive *prim0 = &tri0->mPrim;
	Primitive *prim1 = &tri1->mPrim;
	Primitive *prim2 = &tri2->mPrim;
	Primitive *prim3 = &tri3->mPrim;

	vsOutput *v00, *v01, *v02;
	vsOutput *v10, *v11, *v12;
	vsOutput *v20, *v21, *v22;
	vsOutput *v30, *v31, *v32;

	if (prim0->mAreaReciprocal > 0.0f)
	{
		v00 = &prim0->mVert[0];
		v01 = &prim0->mVert[1];
		v02 = &prim0->mVert[2];
	}
	else
	{
		v00 = &prim0->mVert[2];
		v01 = &prim0->mVert[1];
		v02 = &prim0->mVert[0];
	}
	tri0->mVert2 = v02;

	if (prim1->mAreaReciprocal > 0.0f)
	{
		v10 = &prim1->mVert[0];
		v11 = &prim1->mVert[1];
		v12 = &prim1->mVert[2];
	}
	else
	{
		v10 = &prim1->mVert[2];
		v11 = &prim1->mVert[1];
		v12 = &prim1->mVert[0];
	}
	tri1->mVert2 = v12;

	if (prim2->mAreaReciprocal > 0.0f)
	{
		v20 = &prim2->mVert[0];
		v21 = &prim2->mVert[1];
		v22 = &prim2->mVert[2];
	}
	else
	{
		v20 = &prim2->mVert[2];
		v21 = &prim2->mVert[1];
		v22 = &prim2->mVert[0];
	}
	tri2->mVert2 = v22;

	if (prim3->mAreaReciprocal > 0.0f)
	{
		v30 = &prim3->mVert[0];
		v31 = &prim3->mVert[1];
		v32 = &prim3->mVert[2];
	}
	else
	{
		v30 = &prim3->mVert[2];
		v31 = &prim3->mVert[1];
		v32 = &prim3->mVert[0];
	}
	tri3->mVert2 = v32;

	__m128 vSubpixelsf = _mm_set_ps1(RAST_SUBPIXELS);
	__m128 vX0f = _mm_set_ps(v30->position().x, v20->position().x, v10->position().x, v00->position().x);
	__m128 vY0f = _mm_set_ps(v30->position().y, v20->position().y, v10->position().y, v00->position().y);
	__m128 vX1f = _mm_set_ps(v31->position().x, v21->position().x, v11->position().x, v01->position().x);
	__m128 vY1f = _mm_set_ps(v31->position().y, v21->position().y, v11->position().y, v01->position().y);
	__m128 vX2f = _mm_set_ps(v32->position().x, v22->position().x, v12->position().x, v02->position().x);
	__m128 vY2f = _mm_set_ps(v32->position().y, v22->position().y, v12->position().y, v02->position().y);

	/* Fixed point rasterization algorithm
	 * FXIME: it's possible to overflow integer's range in current guard band setting.
	 */
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
	__m128i vX0 = _mm_cvtps_epi32(_mm_mul_ps(vX0f, vSubpixelsf));
	__m128i vY0 = _mm_cvtps_epi32(_mm_mul_ps(vY0f, vSubpixelsf));
	__m128i vX1 = _mm_cvtps_epi32(_mm_mul_ps(vX1f, vSubpixelsf));
	__m128i vY1 = _mm_cvtps_epi32(_mm_mul_ps(vY1f, vSubpixelsf));
	__m128i vX2 = _mm_cvtps_epi32(_mm_mul_ps(vX2f, vSubpixelsf));
	__m128i vY2 = _mm_cvtps_epi32(_mm_mul_ps(vY2f, vSubpixelsf));

	__m128i vX1X0 = _mm_sub_epi32(vX1, vX0);
	__m128i vX2X1 = _mm_sub_epi32(vX2, vX1);
	__m128i vX0X2 = _mm_sub_epi32(vX0, vX2);

	__m128i vY0Y1 = _mm_sub_epi32(vY0, vY1);
	__m128i vY1Y2 = _mm_sub_epi32(vY1, vY2);
	__m128i vY2Y0 = _mm_sub_epi32(vY2, vY0);

	__m128i vC0 = _mm_sub_epi32(_mm_mullo_epi32(vX1, vY2), _mm_mullo_epi32(vX2, vY1));
	__m128i vC1 = _mm_sub_epi32(_mm_mullo_epi32(vX2, vY0), _mm_mullo_epi32(vX0, vY2));
	__m128i vC2 = _mm_sub_epi32(_mm_mullo_epi32(vX0, vY1), _mm_mullo_epi32(vX1, vY0));

	__m128i vMask;
	__m128i vMask1;
	__m128i vMaskedOne;

	/* Apply top-left filling convention
	 * It's worth memtioning that for shared vertices case,
	 * the vertices will be drawn only if them are shared
	 * between top left edges, i.e. top-left or left-left.
	 * Other kind of shared vertices, like top-right, left-bottom,
	 * will all be abandoned.
	 */
	vMask  = _mm_cmpgt_epi32(vY1Y2, _mm_setzero_si128());
	vMask1 = _mm_and_si128(_mm_cmpeq_epi32(vY1Y2, _mm_setzero_si128()), _mm_cmplt_epi32(vX2X1, _mm_setzero_si128()));
	vMask  = _mm_or_si128(vMask, vMask1);
	vMaskedOne = _mm_and_si128(vMask, _mm_set1_epi32(1));
	vC0  = _mm_sub_epi32(vC0, vMaskedOne);


	vMask  = _mm_cmpgt_epi32(vY2Y0, _mm_setzero_si128());
	vMask1 = _mm_and_si128(_mm_cmpeq_epi32(vY2Y0, _mm_setzero_si128()), _mm_cmplt_epi32(vX0X2, _mm_setzero_si128()));
	vMask  = _mm_or_si128(vMask, vMask1);
	vMaskedOne = _mm_and_si128(vMask, _mm_set1_epi32(1));
	vC1  = _mm_sub_epi32(vC1, vMaskedOne);


	vMask  = _mm_cmpgt_epi32(vY0Y1, _mm_setzero_si128());
	vMask1 = _mm_and_si128(_mm_cmpeq_epi32(vY0Y1, _mm_setzero_si128()), _mm_cmplt_epi32(vX1X0, _mm_setzero_si128()));
	vMask  = _mm_or_si128(vMask, vMask1);
	vMaskedOne = _mm_and_si128(vMask, _mm_set1_epi32(1));
	vC2  = _mm_sub_epi32(vC2, vMaskedOne);


	ALIGN(16) int tmp[4];
	__m128i vFactor;

	// vertex 0: mFactorA
	vFactor = _mm_slli_epi32(vY1Y2, RAST_SUBPIXEL_BITS);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[0].mFactorA = tmp[0];
	tri1->mVert[0].mFactorA = tmp[1];
	tri2->mVert[0].mFactorA = tmp[2];
	tri3->mVert[0].mFactorA = tmp[3];

	// vertex 0: mFactorB
	vFactor = _mm_slli_epi32(vX2X1, RAST_SUBPIXEL_BITS);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[0].mFactorB = tmp[0];
	tri1->mVert[0].mFactorB = tmp[1];
	tri2->mVert[0].mFactorB = tmp[2];
	tri3->mVert[0].mFactorB = tmp[3];


	/* Add an offset to C0, because, we need take into
	 * account the conversion between fixed-point coordinates
	 * (origin is the left bottom of the screen) and the pixel
	 * coordinates(center of a pixel grid).
	 */
	// vertex 0: mFactorC
	vFactor = _mm_add_epi32(_mm_slli_epi32(vY1Y2, RAST_SUBPIXEL_BITS - 1),
							_mm_slli_epi32(vX2X1, RAST_SUBPIXEL_BITS - 1));
	vFactor = _mm_add_epi32(vFactor, vC0);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[0].mFactorC = tmp[0];
	tri1->mVert[0].mFactorC = tmp[1];
	tri2->mVert[0].mFactorC = tmp[2];
	tri3->mVert[0].mFactorC = tmp[3];

	// vertex 1: mFactorA
	vFactor = _mm_slli_epi32(vY2Y0, RAST_SUBPIXEL_BITS);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[1].mFactorA = tmp[0];
	tri1->mVert[1].mFactorA = tmp[1];
	tri2->mVert[1].mFactorA = tmp[2];
	tri3->mVert[1].mFactorA = tmp[3];

	// vertex 1: mFactorB
	vFactor = _mm_slli_epi32(vX0X2, RAST_SUBPIXEL_BITS);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[1].mFactorB = tmp[0];
	tri1->mVert[1].mFactorB = tmp[1];
	tri2->mVert[1].mFactorB = tmp[2];
	tri3->mVert[1].mFactorB = tmp[3];

	// vertex 1: mFactorC
	vFactor = _mm_add_epi32(_mm_slli_epi32(vY2Y0, RAST_SUBPIXEL_BITS - 1),
							_mm_slli_epi32(vX0X2, RAST_SUBPIXEL_BITS - 1));
	vFactor = _mm_add_epi32(vFactor, vC1);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[1].mFactorC = tmp[0];
	tri1->mVert[1].mFactorC = tmp[1];
	tri2->mVert[1].mFactorC = tmp[2];
	tri3->mVert[1].mFactorC = tmp[3];

	// vertex 2: mFactorA
	vFactor = _mm_slli_epi32(vY0Y1, RAST_SUBPIXEL_BITS);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[2].mFactorA = tmp[0];
	tri1->mVert[2].mFactorA = tmp[1];
	tri2->mVert[2].mFactorA = tmp[2];
	tri3->mVert[2].mFactorA = tmp[3];

	// vertex 2: mFactorB
	vFactor = _mm_slli_epi32(vX1X0, RAST_SUBPIXEL_BITS);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[2].mFactorB = tmp[0];
	tri1->mVert[2].mFactorB = tmp[1];
	tri2->mVert[2].mFactorB = tmp[2];
	tri3->mVert[2].mFactorB = tmp[3];

	// vertex 2: mFactorC
	vFactor = _mm_add_epi32(_mm_slli_epi32(vY0Y1, RAST_SUBPIXEL_BITS - 1),
							_mm_slli_epi32(vX1X0, RAST_SUBPIXEL_BITS - 1));
	vFactor = _mm_add_epi32(vFactor, vC2);
	_mm_store_si128((__m128i *)tmp, vFactor);
	tri0->mVert[2].mFactorC = tmp[0];
	tri1->mVert[2].mFactorC = tmp[1];
	tri2->mVert[2].mFactorC = tmp[2];
	tri3->mVert[2].mFactorC = tmp[3];


	__m128i vSubpixels = _mm_set1_epi32(RAST_SUBPIXELS);
	__m128i vXmin = _mm_srli_epi32(_mm_min_epi32(_mm_min_epi32(vX0, vX1), vX2), RAST_SUBPIXEL_BITS);
	vXmin = _mm_min_epi32(_mm_max_epi32(vXmin, _mm_setzero_si128()), _mm_set1_epi32(g_GC->mRT.width - 1));
	_mm_store_si128((__m128i *)tmp, vXmin);
	tri0->xmin = tmp[0];
	tri1->xmin = tmp[1];
	tri2->xmin = tmp[2];
	tri3->xmin = tmp[3];

	__m128i vYmin = _mm_srli_epi32(_mm_min_epi32(_mm_min_epi32(vY0, vY1), vY2), RAST_SUBPIXEL_BITS);
	vYmin = _mm_min_epi32(_mm_max_epi32(vYmin, _mm_setzero_si128()), _mm_set1_epi32(g_GC->mRT.height - 1));
	_mm_store_si128((__m128i *)tmp, vYmin);
	tri0->ymin = tmp[0];
	tri1->ymin = tmp[1];
	tri2->ymin = tmp[2];
	tri3->ymin = tmp[3];

	__m128i vXmax = _mm_max_epi32(_mm_max_epi32(vX0, vX1), vX2);
	vXmax = _mm_sub_epi32(_mm_srli_epi32(ROUND_UP_SIMD(vXmax, vSubpixels), RAST_SUBPIXEL_BITS), _mm_set1_epi32(1));
	vXmax = _mm_min_epi32(_mm_max_epi32(vXmax, _mm_setzero_si128()), _mm_set1_epi32(g_GC->mRT.width - 1));
	_mm_store_si128((__m128i *)tmp, vXmax);
	tri0->xmax = tmp[0];
	tri1->xmax = tmp[1];
	tri2->xmax = tmp[2];
	tri3->xmax = tmp[3];

	__m128i vYmax = _mm_max_epi32(_mm_max_epi32(vY0, vY1), vY2);
	vYmax = _mm_sub_epi32(_mm_srli_epi32(ROUND_UP_SIMD(vYmax, vSubpixels), RAST_SUBPIXEL_BITS), _mm_set1_epi32(1));
	vYmax = _mm_min_epi32(_mm_max_epi32(vYmax, _mm_setzero_si128()), _mm_set1_epi32(g_GC->mRT.height - 1));
	_mm_store_si128((__m128i *)tmp, vYmax);
	tri0->ymax = tmp[0];
	tri1->ymax = tmp[1];
	tri2->ymax = tmp[2];
	tri3->ymax = tmp[3];


	__m128 vWReciprocal0 = _mm_rcp_ps(_mm_set_ps(v30->position().w, v20->position().w, v10->position().w, v00->position().w));
	__m128 vWReciprocal1 = _mm_rcp_ps(_mm_set_ps(v31->position().w, v21->position().w, v11->position().w, v01->position().w));
	__m128 vWReciprocal2 = _mm_rcp_ps(_mm_set_ps(v32->position().w, v22->position().w, v12->position().w, v02->position().w));

	__m128 vAreaRecip = _mm_set_ps(prim3->mAreaReciprocal,
								prim2->mAreaReciprocal,
								prim1->mAreaReciprocal,
								prim0->mAreaReciprocal);
	vAreaRecip = _mm_andnot_ps(_mm_set_ps1(-0.0f), vAreaRecip);

	__m128 vY1Y2f = _mm_mul_ps(_mm_sub_ps(vY1f, vY2f), vAreaRecip);
	__m128 vY2Y0f = _mm_mul_ps(_mm_sub_ps(vY2f, vY0f), vAreaRecip);
	__m128 vY0Y1f = _mm_mul_ps(_mm_sub_ps(vY0f, vY1f), vAreaRecip);

	__m128 vX2X1f = _mm_mul_ps(_mm_sub_ps(vX2f, vX1f), vAreaRecip);
	__m128 vX0X2f = _mm_mul_ps(_mm_sub_ps(vX0f, vX2f), vAreaRecip);
	__m128 vX1X0f = _mm_mul_ps(_mm_sub_ps(vX1f, vX0f), vAreaRecip);

	__m128 vLambdax0 = _mm_mul_ps(vY1Y2f, vWReciprocal0);
	__m128 vLambdax1 = _mm_mul_ps(vY2Y0f, vWReciprocal1);
	__m128 vLambdax2 = _mm_mul_ps(vY0Y1f, vWReciprocal2);

	__m128 vLambday0 = _mm_mul_ps(vX2X1f, vWReciprocal0);
	__m128 vLambday1 = _mm_mul_ps(vX0X2f, vWReciprocal1);
	__m128 vLambday2 = _mm_mul_ps(vX1X0f, vWReciprocal2);

	__m128 vXoffset = _mm_sub_ps(vX0f, _mm_set_ps1(0.5f));
	__m128 vYoffset = _mm_sub_ps(vY0f, _mm_set_ps1(0.5f));

	ALIGN(16) float tmpf[4];

	_mm_store_ps(tmpf, vLambdax0);
	tri0->mPCBCOnW0GradientX = tmpf[0];
	tri1->mPCBCOnW0GradientX = tmpf[1];
	tri2->mPCBCOnW0GradientX = tmpf[2];
	tri3->mPCBCOnW0GradientX = tmpf[3];

	_mm_store_ps(tmpf, vLambday0);
	tri0->mPCBCOnW0GradientY = tmpf[0];
	tri1->mPCBCOnW0GradientY = tmpf[1];
	tri2->mPCBCOnW0GradientY = tmpf[2];
	tri3->mPCBCOnW0GradientY = tmpf[3];

	__m128 vPCBCOnW0AtOrigin = _mm_sub_ps(_mm_sub_ps(vWReciprocal0, _mm_mul_ps(vLambdax0, vXoffset)),
										_mm_mul_ps(vLambday0, vYoffset));
	_mm_store_ps(tmpf, vPCBCOnW0AtOrigin);
	tri0->mPCBCOnW0AtOrigin = tmpf[0];
	tri1->mPCBCOnW0AtOrigin = tmpf[1];
	tri2->mPCBCOnW0AtOrigin = tmpf[2];
	tri3->mPCBCOnW0AtOrigin = tmpf[3];

	_mm_store_ps(tmpf, vLambdax1);
	tri0->mPCBCOnW1GradientX = tmpf[0];
	tri1->mPCBCOnW1GradientX = tmpf[1];
	tri2->mPCBCOnW1GradientX = tmpf[2];
	tri3->mPCBCOnW1GradientX = tmpf[3];

	_mm_store_ps(tmpf, vLambday1);
	tri0->mPCBCOnW1GradientY = tmpf[0];
	tri1->mPCBCOnW1GradientY = tmpf[1];
	tri2->mPCBCOnW1GradientY = tmpf[2];
	tri3->mPCBCOnW1GradientY = tmpf[3];

	__m128 vPCBCOnW1AtOrigin = _mm_sub_ps(_mm_sub_ps(_mm_setzero_ps(), _mm_mul_ps(vLambdax1, vXoffset)),
										_mm_mul_ps(vLambday1, vYoffset));
	_mm_store_ps(tmpf, vPCBCOnW1AtOrigin);
	tri0->mPCBCOnW1AtOrigin = tmpf[0];
	tri1->mPCBCOnW1AtOrigin = tmpf[1];
	tri2->mPCBCOnW1AtOrigin = tmpf[2];
	tri3->mPCBCOnW1AtOrigin = tmpf[3];

	__m128 vWRecipGradientX = _mm_add_ps(_mm_add_ps(vLambdax0, vLambdax1), vLambdax2);
	_mm_store_ps(tmpf, vWRecipGradientX);
	tri0->mWRecipGradientX = tmpf[0];
	tri1->mWRecipGradientX = tmpf[1];
	tri2->mWRecipGradientX = tmpf[2];
	tri3->mWRecipGradientX = tmpf[3];

	__m128 vWRecipGradientY = _mm_add_ps(_mm_add_ps(vLambday0, vLambday1), vLambday2);
	_mm_store_ps(tmpf, vWRecipGradientY);
	tri0->mWRecipGradientY = tmpf[0];
	tri1->mWRecipGradientY = tmpf[1];
	tri2->mWRecipGradientY = tmpf[2];
	tri3->mWRecipGradientY = tmpf[3];

	__m128 vWRecipAtOrigin = _mm_sub_ps(_mm_sub_ps(vWReciprocal0, _mm_mul_ps(vWRecipGradientX, vXoffset)),
										_mm_mul_ps(vWRecipGradientY, vYoffset));
	_mm_store_ps(tmpf, vWRecipAtOrigin);
	tri0->mWRecipAtOrigin = tmpf[0];
	tri1->mWRecipAtOrigin = tmpf[1];
	tri2->mWRecipAtOrigin = tmpf[2];
	tri3->mWRecipAtOrigin = tmpf[3];

	__m128 vZ0f = _mm_set_ps(v30->position().z, v20->position().z, v10->position().z, v00->position().z);
	__m128 vZ1f = _mm_set_ps(v31->position().z, v21->position().z, v11->position().z, v01->position().z);
	__m128 vZ2f = _mm_set_ps(v32->position().z, v22->position().z, v12->position().z, v02->position().z);
	__m128 vZGradientX = _mm_add_ps(_mm_add_ps(_mm_mul_ps(vY1Y2f, vZ0f), _mm_mul_ps(vY2Y0f, vZ1f)),
									_mm_mul_ps(vY0Y1f, vZ2f));
	__m128 vZGradientY = _mm_add_ps(_mm_add_ps(_mm_mul_ps(vX2X1f, vZ0f), _mm_mul_ps(vX0X2f, vZ1f)),
									_mm_mul_ps(vX1X0f, vZ2f));
	__m128 vZAtOrigin = _mm_sub_ps(_mm_sub_ps(vZ0f, _mm_mul_ps(vZGradientX, vXoffset)),
								_mm_mul_ps(vZGradientY, vYoffset));

	_mm_store_ps(tmpf, vZGradientX);
	tri0->mZGradientX = tmpf[0];
	tri1->mZGradientX = tmpf[1];
	tri2->mZGradientX = tmpf[2];
	tri3->mZGradientX = tmpf[3];

	_mm_store_ps(tmpf, vZGradientY);
	tri0->mZGradientY = tmpf[0];
	tri1->mZGradientY = tmpf[1];
	tri2->mZGradientY = tmpf[2];
	tri3->mZGradientY = tmpf[3];

	_mm_store_ps(tmpf, vZAtOrigin);
	tri0->mZAtOrigin = tmpf[0];
	tri1->mZAtOrigin = tmpf[1];
	tri2->mZAtOrigin = tmpf[2];
	tri3->mZAtOrigin = tmpf[3];


	// FIXME: attributes size of these triangles may be different.
	const size_t size = v00->getRegsNum();
	tri0->mAttrPlaneEquationA.resize(size);
	tri0->mAttrPlaneEquationB.resize(size);
	tri1->mAttrPlaneEquationA.resize(size);
	tri1->mAttrPlaneEquationB.resize(size);
	tri2->mAttrPlaneEquationA.resize(size);
	tri2->mAttrPlaneEquationB.resize(size);
	tri3->mAttrPlaneEquationA.resize(size);
	tri3->mAttrPlaneEquationB.resize(size);

	__m128 vAttr0;
	__m128 vAttr1;
	__m128 vAttr2;
	for (size_t i = 1; i < size; ++i)
	{
		vAttr0 = _mm_load_ps((float *)&v00->getReg(i));
		vAttr1 = _mm_load_ps((float *)&v01->getReg(i));
		vAttr2 = _mm_load_ps((float *)&v02->getReg(i));

		_mm_store_ps((float *)&(tri0->mAttrPlaneEquationA.getReg(i)), _mm_sub_ps(vAttr0, vAttr2));
		_mm_store_ps((float *)&(tri0->mAttrPlaneEquationB.getReg(i)), _mm_sub_ps(vAttr1, vAttr2));

		vAttr0 = _mm_load_ps((float *)&v10->getReg(i));
		vAttr1 = _mm_load_ps((float *)&v11->getReg(i));
		vAttr2 = _mm_load_ps((float *)&v12->getReg(i));

		_mm_store_ps((float *)&(tri1->mAttrPlaneEquationA.getReg(i)), _mm_sub_ps(vAttr0, vAttr2));
		_mm_store_ps((float *)&(tri1->mAttrPlaneEquationB.getReg(i)), _mm_sub_ps(vAttr1, vAttr2));

		vAttr0 = _mm_load_ps((float *)&v20->getReg(i));
		vAttr1 = _mm_load_ps((float *)&v21->getReg(i));
		vAttr2 = _mm_load_ps((float *)&v22->getReg(i));

		_mm_store_ps((float *)&(tri2->mAttrPlaneEquationA.getReg(i)), _mm_sub_ps(vAttr0, vAttr2));
		_mm_store_ps((float *)&(tri2->mAttrPlaneEquationB.getReg(i)), _mm_sub_ps(vAttr1, vAttr2));

		vAttr0 = _mm_load_ps((float *)&v30->getReg(i));
		vAttr1 = _mm_load_ps((float *)&v31->getReg(i));
		vAttr2 = _mm_load_ps((float *)&v32->getReg(i));

		_mm_store_ps((float *)&(tri3->mAttrPlaneEquationA.getReg(i)), _mm_sub_ps(vAttr0, vAttr2));
		_mm_store_ps((float *)&(tri3->mAttrPlaneEquationB.getReg(i)), _mm_sub_ps(vAttr1, vAttr2));
	}
}

void Binning::SetupTriangle(Triangle *tri)
{
	Primitive &prim = tri->mPrim;

	vsOutput *v0;
	vsOutput *v1;
	vsOutput *v2;

	// Always make (v0,v1,v2) counter-closewise
	if (prim.mAreaReciprocal > 0.0f)
	{
		v0 = &prim.mVert[0];
		v1 = &prim.mVert[1];
		v2 = &prim.mVert[2];
	}
	else
	{
		v0 = &prim.mVert[2];
		v1 = &prim.mVert[1];
		v2 = &prim.mVert[0];
	}

	tri->mVert2 = v2;

	/* Fixed point rasterization algorithm
	 */
	const int x0 = fixedpoint_cast<RAST_SUBPIXELS>(v0->position().x);
	const int y0 = fixedpoint_cast<RAST_SUBPIXELS>(v0->position().y);

	const int x1 = fixedpoint_cast<RAST_SUBPIXELS>(v1->position().x);
	const int y1 = fixedpoint_cast<RAST_SUBPIXELS>(v1->position().y);

	const int x2 = fixedpoint_cast<RAST_SUBPIXELS>(v2->position().x);
	const int y2 = fixedpoint_cast<RAST_SUBPIXELS>(v2->position().y);

	const int x1x0 = x1 - x0;
	const int x2x1 = x2 - x1;
	const int x0x2 = x0 - x2;

	const int y0y1 = y0 - y1;
	const int y1y2 = y1 - y2;
	const int y2y0 = y2 - y0;

	int C0 = x1 * y2 - x2 * y1;
	int C1 = x2 * y0 - x0 * y2;
	int C2 = x0 * y1 - x1 * y0;

	/* Apply top-left filling convention
	 * It's worth memtioning that for shared vertices case,
	 * the vertices will be drawn only if them are shared
	 * between top left edges, i.e. top-left or left-left.
	 * Other kind of shared vertices, like top-right, left-bottom,
	 * will all be abandoned.
	 */
	if (y1y2 > 0 || (y1y2 == 0 && x2x1 < 0))  C0--;
	if (y2y0 > 0 || (y2y0 == 0 && x0x2 < 0))  C1--;
	if (y0y1 > 0 || (y0y1 == 0 && x1x0 < 0))  C2--;


	/* We add an offset to C0, because, we need take into
	 * account the conversion between fixed-point coordinates
	 * (origin is the left bottom of the screen) and the pixel
	 * coordinates(center of a pixel grid).
	 */
	tri->mVert[0].mFactorA = y1y2 << RAST_SUBPIXEL_BITS;
	tri->mVert[0].mFactorB = x2x1 << RAST_SUBPIXEL_BITS;
	tri->mVert[0].mFactorC = C0 + (y1y2 << (RAST_SUBPIXEL_BITS - 1)) + (x2x1 << (RAST_SUBPIXEL_BITS - 1));

	tri->mVert[1].mFactorA = y2y0 << RAST_SUBPIXEL_BITS;
	tri->mVert[1].mFactorB = x0x2 << RAST_SUBPIXEL_BITS;
	tri->mVert[1].mFactorC = C1 + (y2y0 << (RAST_SUBPIXEL_BITS - 1)) + (x0x2 << (RAST_SUBPIXEL_BITS - 1));

	tri->mVert[2].mFactorA = y0y1 << RAST_SUBPIXEL_BITS;
	tri->mVert[2].mFactorB = x1x0 << RAST_SUBPIXEL_BITS;
	tri->mVert[2].mFactorC = C2 + (y0y1 << (RAST_SUBPIXEL_BITS - 1)) + (x1x0 << (RAST_SUBPIXEL_BITS - 1));

	tri->xmin = (std::min({x0, x1, x2}) >> RAST_SUBPIXEL_BITS);
	tri->ymin = (std::min({y0, y1, y2}) >> RAST_SUBPIXEL_BITS);
	tri->xmax = ((ROUND_UP(std::max({x0, x1, x2}), RAST_SUBPIXELS) >> RAST_SUBPIXEL_BITS) - 1);
	tri->ymax = ((ROUND_UP(std::max({y0, y1, y2}), RAST_SUBPIXELS) >> RAST_SUBPIXEL_BITS) - 1);

	tri->xmin = clamp(tri->xmin, 0, g_GC->mRT.width  - 1);
	tri->xmax = clamp(tri->xmax, 0, g_GC->mRT.width  - 1);
	tri->ymin = clamp(tri->ymin, 0, g_GC->mRT.height - 1);
	tri->ymax = clamp(tri->ymax, 0, g_GC->mRT.height - 1);

	const float WReciprocal0 = 1.0f / v0->position().w;
	const float WReciprocal1 = 1.0f / v1->position().w;
	const float WReciprocal2 = 1.0f / v2->position().w;

	const float area_reciprocal = fabs(prim.mAreaReciprocal);

	const float y1y2f = (v1->position().y - v2->position().y) * area_reciprocal;
	const float y2y0f = (v2->position().y - v0->position().y) * area_reciprocal;
	const float y0y1f = (v0->position().y - v1->position().y) * area_reciprocal;

	const float x2x1f = (v2->position().x - v1->position().x) * area_reciprocal;
	const float x0x2f = (v0->position().x - v2->position().x) * area_reciprocal;
	const float x1x0f = (v1->position().x - v0->position().x) * area_reciprocal;

	const float lambdax0 = y1y2f * WReciprocal0;
	const float lambdax1 = y2y0f * WReciprocal1;
	const float lambdax2 = y0y1f * WReciprocal2;

	const float lambday0 = x2x1f * WReciprocal0;
	const float lambday1 = x0x2f * WReciprocal1;
	const float lambday2 = x1x0f * WReciprocal2;

	// Substracted by 0.5f because of the conversion b/w
	// pixel index and window coodinates, this avoid per-pixel
	// substraction.
	const float xoffset = v0->position().x - 0.5f;
	const float yoffset = v0->position().y - 0.5f;

	tri->mPCBCOnW0GradientX = lambdax0;
	tri->mPCBCOnW0GradientY = lambday0;
	tri->mPCBCOnW0AtOrigin  = WReciprocal0 - tri->mPCBCOnW0GradientX * xoffset - tri->mPCBCOnW0GradientY * yoffset;

	tri->mPCBCOnW1GradientX = lambdax1;
	tri->mPCBCOnW1GradientY = lambday1;
	tri->mPCBCOnW1AtOrigin  = -(tri->mPCBCOnW1GradientX) * xoffset - tri->mPCBCOnW1GradientY * yoffset;

	tri->mWRecipGradientX   = lambdax0 + lambdax1 + lambdax2;
	tri->mWRecipGradientY   = lambday0 + lambday1 + lambday2;
	tri->mWRecipAtOrigin    = WReciprocal0 - tri->mWRecipGradientX * xoffset - tri->mWRecipGradientY * yoffset;

	tri->mZGradientX = y1y2f * v0->position().z + y2y0f * v1->position().z + y0y1f * v2->position().z;
	tri->mZGradientY = x2x1f * v0->position().z + x0x2f * v1->position().z + x1x0f * v2->position().z;
	tri->mZAtOrigin  = v0->position().z - tri->mZGradientX * xoffset - tri->mZGradientY * yoffset;

	const size_t size = v0->getRegsNum();
	tri->mAttrPlaneEquationA.resize(size);
	tri->mAttrPlaneEquationB.resize(size);

	for (size_t i = 1; i < size; ++i)
	{
		tri->mAttrPlaneEquationA.getReg(i) = v0->getReg(i) - v2->getReg(i);
		tri->mAttrPlaneEquationB.getReg(i) = v1->getReg(i) - v2->getReg(i);
	}
}


void Binning::CoarseRasterizing(Triangle *tri)
{
	const int xmin = ROUND_DOWN(tri->xmin, MACRO_TILE_SIZE);
	const int ymin = ROUND_DOWN(tri->ymin, MACRO_TILE_SIZE);

	__m128i vFactorA0 = _mm_set1_epi32(tri->mVert[0].mFactorA);
	__m128i vFactorB0 = _mm_set1_epi32(tri->mVert[0].mFactorB);
	__m128i vFactorC0 = _mm_set1_epi32(tri->mVert[0].mFactorC);

	__m128i vFactorA1 = _mm_set1_epi32(tri->mVert[1].mFactorA);
	__m128i vFactorB1 = _mm_set1_epi32(tri->mVert[1].mFactorB);
	__m128i vFactorC1 = _mm_set1_epi32(tri->mVert[1].mFactorC);

	__m128i vFactorA2 = _mm_set1_epi32(tri->mVert[2].mFactorA);
	__m128i vFactorB2 = _mm_set1_epi32(tri->mVert[2].mFactorB);
	__m128i vFactorC2 = _mm_set1_epi32(tri->mVert[2].mFactorC);

	for (int y = ymin; y <= tri->ymax; y += MACRO_TILE_SIZE)
	{
		for (int x = xmin; x <= tri->xmax; x += MACRO_TILE_SIZE)
		{
			__m128i vTileCornerX = _mm_set_epi32(x, x + MACRO_TILE_SIZE - 1, x, x + MACRO_TILE_SIZE - 1);
			__m128i vTileCornerY = _mm_set_epi32(y, y, y + MACRO_TILE_SIZE - 1, y + MACRO_TILE_SIZE - 1);

			__m128i vArea0 = MAWrapper(vFactorB0, vTileCornerY, MAWrapper(vFactorA0, vTileCornerX, vFactorC0));
			__m128i vTest0 = _mm_cmplt_epi32(vArea0, _mm_setzero_si128());
			// This macro tile is totally outside the triangle
			if (_mm_test_all_ones(vTest0))
				continue;

			__m128i vArea1 = MAWrapper(vFactorB1, vTileCornerY, MAWrapper(vFactorA1, vTileCornerX, vFactorC1));
			__m128i vTest1 = _mm_cmplt_epi32(vArea1, _mm_setzero_si128());
			if (_mm_test_all_ones(vTest1))
				continue;

			__m128i vArea2 = MAWrapper(vFactorB2, vTileCornerY, MAWrapper(vFactorA2, vTileCornerX, vFactorC2));
			__m128i vTest2 = _mm_cmplt_epi32(vArea2, _mm_setzero_si128());
			if (_mm_test_all_ones(vTest2))
				continue;

			// This macro tile is totally inside the triangle
			if (_mm_test_all_zeros(vTest0, _mm_set1_epi32(0xFFFFFFFF)) &&
				_mm_test_all_zeros(vTest1, _mm_set1_epi32(0xFFFFFFFF)) &&
				_mm_test_all_zeros(vTest2, _mm_set1_epi32(0xFFFFFFFF)))
			{
				s_FullCoverDispListLock.lock();
				s_FullCoverDispList[y >> MACRO_TILE_SIZE_SHIFT][x >> MACRO_TILE_SIZE_SHIFT].push_back(tri);
				s_FullCoverDispListLock.unlock();
			}
			// This macro tile totally contain the triangle(can do OPT ?),
			// or it's clipped against the triangle edges, so need further rasterization
			else
			{
				s_DispListLock.lock();
				s_DispList[y >> MACRO_TILE_SIZE_SHIFT][x >> MACRO_TILE_SIZE_SHIFT].push_back(tri);
				s_DispListLock.unlock();
			}
		}
	}
}

void Binning::finalize()
{
}

static inline void CalculatePlaneEquation(__m128 &A, __m128 &lambda0, __m128 &B, __m128 &lambda1, __m128 &C)
{
#if defined(__AVX2_)
	// FMA, relaxed floating-point precision
	C = _mm_fmadd_ps(A, lambda0, C);
	C = _mm_fmadd_ps(B, lambda1, C);
#else 
	__m128 vTmp = _mm_mul_ps(A, lambda0);
	C           = _mm_add_ps(C, vTmp);
	vTmp        = _mm_mul_ps(B, lambda1);
	C           = _mm_add_ps(C, vTmp);
#endif
}

void PerspectiveCorrectInterpolater::emit(void *data)
{
	//onInterpolatingSISD(data);
	onInterpolatingSIMD(data);
}

void PerspectiveCorrectInterpolater::onInterpolatingSISD(void *data)
{
	Fsio &fsio = *static_cast<Fsio *>(data);
	const Triangle *tri = static_cast<Triangle *>(fsio.m_priv0);
	size_t size = tri->mPrim.mVert[0].getRegsNum();

	const float &stepx = (float)fsio.x;
	const float &stepy = (float)fsio.y;
	float w     = tri->mWRecipGradientX * stepx + tri->mWRecipGradientY * stepy + tri->mWRecipAtOrigin;
	w = 1.0f / w;

	float pcbc0 = tri->mPCBCOnW0GradientX * stepx + tri->mPCBCOnW0GradientY * stepy + tri->mPCBCOnW0AtOrigin;
	float pcbc1 = tri->mPCBCOnW1GradientX * stepx + tri->mPCBCOnW1GradientY * stepy + tri->mPCBCOnW1AtOrigin;

	pcbc0 *= w;
	pcbc1 *= w;

	for (size_t i = 1; i < size; ++i)
	{
		fsio.in[i] = tri->mVert2->getReg(i) + tri->mAttrPlaneEquationA[i] * pcbc0 + tri->mAttrPlaneEquationB[i] * pcbc1;
	}
}

void PerspectiveCorrectInterpolater::onInterpolatingSIMD(void *data)
{
	Fsiosimd &fsio = *static_cast<Fsiosimd *>(data);
	const Triangle *tri = static_cast<Triangle *>(fsio.m_priv0);
	size_t size = tri->mPrim.mVert[0].getRegsNum();
	__m128 vX = _mm_cvtepi32_ps(_mm_set_epi32(fsio.x, fsio.x + 1, fsio.x,     fsio.x + 1));
	__m128 vY = _mm_cvtepi32_ps(_mm_set_epi32(fsio.y, fsio.y,     fsio.y + 1, fsio.y + 1));

	__m128 vGradX = _mm_set_ps1(tri->mWRecipGradientX);
	__m128 vGradY = _mm_set_ps1(tri->mWRecipGradientY);
	__m128 vW     = _mm_set_ps1(tri->mWRecipAtOrigin);
	CalculatePlaneEquation(vGradX, vX, vGradY, vY, vW);
	vW = _mm_rcp_ps(vW);

	vGradX        = _mm_set_ps1(tri->mPCBCOnW0GradientX);
	vGradY        = _mm_set_ps1(tri->mPCBCOnW0GradientY);
	__m128 vPCBC0 = _mm_set_ps1(tri->mPCBCOnW0AtOrigin);
	CalculatePlaneEquation(vGradX, vX, vGradY, vY, vPCBC0);
	vPCBC0 = _mm_mul_ps(vPCBC0, vW);

	vGradX        = _mm_set_ps1(tri->mPCBCOnW1GradientX);
	vGradY        = _mm_set_ps1(tri->mPCBCOnW1GradientY);
	__m128 vPCBC1 = _mm_set_ps1(tri->mPCBCOnW1AtOrigin);
	CalculatePlaneEquation(vGradX, vX, vGradY, vY, vPCBC1);
	vPCBC1 = _mm_mul_ps(vPCBC1, vW);

	__m128 vRes;
	__m128 vAPEA, vAPEB;
	for (size_t i = 1; i < size; ++i)
	{
		vRes   = _mm_set_ps1(tri->mVert2->getReg(i).x);
		vAPEA = _mm_set_ps1(tri->mAttrPlaneEquationA[i].x);
		vAPEB = _mm_set_ps1(tri->mAttrPlaneEquationB[i].x);
		CalculatePlaneEquation(vAPEA, vPCBC0, vAPEB, vPCBC1, vRes);
		_mm_store_ps((float *)&fsio.mInRegs[4 * i + 0], vRes);

		vRes   = _mm_set_ps1(tri->mVert2->getReg(i).y);
		vAPEA = _mm_set_ps1(tri->mAttrPlaneEquationA[i].y);
		vAPEB = _mm_set_ps1(tri->mAttrPlaneEquationB[i].y);
		CalculatePlaneEquation(vAPEA, vPCBC0, vAPEB, vPCBC1, vRes);
		_mm_store_ps((float *)&fsio.mInRegs[4 * i + 1], vRes);

		vRes   = _mm_set_ps1(tri->mVert2->getReg(i).z);
		vAPEA = _mm_set_ps1(tri->mAttrPlaneEquationA[i].z);
		vAPEB = _mm_set_ps1(tri->mAttrPlaneEquationB[i].z);
		CalculatePlaneEquation(vAPEA, vPCBC0, vAPEB, vPCBC1, vRes);
		_mm_store_ps((float *)&fsio.mInRegs[4 * i + 2], vRes);

		vRes   = _mm_set_ps1(tri->mVert2->getReg(i).w);
		vAPEA = _mm_set_ps1(tri->mAttrPlaneEquationA[i].w);
		vAPEB = _mm_set_ps1(tri->mAttrPlaneEquationB[i].w);
		CalculatePlaneEquation(vAPEA, vPCBC0, vAPEB, vPCBC1, vRes);
		_mm_store_ps((float *)&fsio.mInRegs[4 * i + 3], vRes);
	}
}

void PerspectiveCorrectInterpolater::onInterpolating(
									const fsInput &in,
							 		const fsInput &gradX,
							 		const fsInput &gradY,
							 		float stepX, float stepY,
							 		fsInput &out)
{
	// OPT: performance issue with operator overloading?
	//out = in + gradX * stepX + gradY * stepY;
	size_t size = in.size();

	assert(gradX.size() == size);
	assert(gradY.size() == size);
	assert(out.size() == size);

	for(size_t i = 0; i < size; ++i)
	{
		out[i] = in[i] + gradX[i] * stepX + gradY[i] * stepY;
	}

	float z = 1.0f / out[0].w;
	out[0].w = z;

	for(size_t i = 1; i < size; ++i)
	{
		out[i] *= z;
	}
}

void PerspectiveCorrectInterpolater::onInterpolating(
									const fsInput &in,
							 		const fsInput &grad,
							 		float x,
							 		fsInput &out)
{
	assert(grad.size() == in.size());

	// OPT: performance issue with operator overloading?
	out = in + grad * x;

	float z = 1.0f / out[0].w;
	out[0].w = z;

	for(size_t i = 1; i < in.size(); ++i)
	{
		out[i] *= z;
	}
}

// TODO: SIMD boost
Triangle::Triangle(Primitive &prim):
	mPrim(prim)
{
}

TBDR::TBDR(DrawEngine &de):
	Rasterizer(),
	mDE(de),
	mDepthClearFlag(false),
	mFlushTriggerBySwapBuffer(true)
{
	const int thread_number = ThreadPool::get().getThreadsNumber();

	mPixelPrimMap = (PixelPrimMap *)malloc(sizeof(PixelPrimMap) * thread_number);
	mZBuffer      = (ZBuffer      *)malloc(sizeof(ZBuffer     ) * thread_number);

	assert(mPixelPrimMap && mZBuffer);
}

TBDR::~TBDR()
{
	free(mZBuffer);
	free(mPixelPrimMap);
}

void TBDR::onRasterizing()
{
	::glsp::ThreadPool &thread_pool = ::glsp::ThreadPool::get();

	for (int y = 0; y < MAX_TILES_IN_HEIGHT; ++y)
	{
		for (int x = 0; x < MAX_TILES_IN_WIDTH; ++x)
		{
			std::vector<Triangle *> &disp_list      = s_DispList[y][x];
			std::vector<Triangle *> &full_disp_list = s_FullCoverDispList[y][x];

			if (!disp_list.empty() || !full_disp_list.empty())
			{
				auto task_handler = [this, x, y](void *data)
				{
					this->ProcessMacroTile(x, y);
				};
				WorkItem *task = thread_pool.CreateWork(task_handler, nullptr);
				thread_pool.AddWork(task);
			}
		}
	}
}

// TODO: SIMD boost
void TBDR::ProcessMacroTile(int x, int y)
{
	PixelPrimMap &pp_map = mPixelPrimMap[ThreadPool::getThreadID()];
	ZBuffer      &z_buf  = mZBuffer     [ThreadPool::getThreadID()];

	std::vector<Triangle *> &disp_list      = s_DispList[y][x];
	std::vector<Triangle *> &full_disp_list = s_FullCoverDispList[y][x];

	x = (x << MACRO_TILE_SIZE_SHIFT);
	y = (y << MACRO_TILE_SIZE_SHIFT);

	if (mDepthClearFlag)
	{
		__m128 vDepth = _mm_set_ps1(static_cast<float>(g_GC->mState.mClearState.depth));
		float *addr = &z_buf[0][0];

		// Assume 64 bytes cache line size
		for (int i = 0; i < MACRO_TILE_SIZE * MACRO_TILE_SIZE; i += 16, addr += 16)
		{
			_mm_store_ps(addr     , vDepth);
			_mm_store_ps(addr + 4 , vDepth);
			_mm_store_ps(addr + 8 , vDepth);
			_mm_store_ps(addr + 12, vDepth);
		}
	}
	else
	{
		// TODO: load depth buffer from render target.
	}

	std::memset(&pp_map[0][0], 0, sizeof(PixelPrimMap));

	for (Triangle *tri: full_disp_list)
	{
		float z = tri->mZAtOrigin + tri->mZGradientX * x + tri->mZGradientY * y;

		for (int i = 0; i < MACRO_TILE_SIZE && (y + i) < g_GC->mRT.height; ++i)
		{
			float zx = z;

			for (int j = 0; j < MACRO_TILE_SIZE && (x + j) < g_GC->mRT.width; ++j)
			{
				if (zx < z_buf[i][j])
				{
					z_buf[i][j]  = zx;
					pp_map[i][j] = tri;
				}

				zx += tri->mZGradientX;
			}
			z += tri->mZGradientY;
		}
	}

	for (Triangle *tri: disp_list)
	{
		for (int yp = y, i = 0; i < MICRO_TILES_IN_MACRO_TILE && yp < g_GC->mRT.height; yp += MICRO_TILE_SIZE, ++i)
		{
			for (int xp = x, j = 0; j < MICRO_TILES_IN_MACRO_TILE && xp < g_GC->mRT.width; xp += MICRO_TILE_SIZE, ++j)
			{
				const int & xleft   = xp;
				const int & ybottom = yp;
				const int & xright  = xp + MICRO_TILE_SIZE - 1;
				const int & ytop    = yp + MICRO_TILE_SIZE - 1;

				int left_bottom_v0  = tri->mVert[0].mFactorA * xleft  + tri->mVert[0].mFactorB * ybottom + tri->mVert[0].mFactorC;
				int left_top_v0     = tri->mVert[0].mFactorA * xleft  + tri->mVert[0].mFactorB * ytop    + tri->mVert[0].mFactorC;
				int right_bottom_v0 = tri->mVert[0].mFactorA * xright + tri->mVert[0].mFactorB * ybottom + tri->mVert[0].mFactorC;
				int right_top_v0    = tri->mVert[0].mFactorA * xright + tri->mVert[0].mFactorB * ytop    + tri->mVert[0].mFactorC;

				int left_bottom_v1  = tri->mVert[1].mFactorA * xleft  + tri->mVert[1].mFactorB * ybottom + tri->mVert[1].mFactorC;
				int left_top_v1     = tri->mVert[1].mFactorA * xleft  + tri->mVert[1].mFactorB * ytop    + tri->mVert[1].mFactorC;
				int right_bottom_v1 = tri->mVert[1].mFactorA * xright + tri->mVert[1].mFactorB * ybottom + tri->mVert[1].mFactorC;
				int right_top_v1    = tri->mVert[1].mFactorA * xright + tri->mVert[1].mFactorB * ytop    + tri->mVert[1].mFactorC;

				int left_bottom_v2  = tri->mVert[2].mFactorA * xleft  + tri->mVert[2].mFactorB * ybottom + tri->mVert[2].mFactorC;
				int left_top_v2     = tri->mVert[2].mFactorA * xleft  + tri->mVert[2].mFactorB * ytop    + tri->mVert[2].mFactorC;
				int right_bottom_v2 = tri->mVert[2].mFactorA * xright + tri->mVert[2].mFactorB * ybottom + tri->mVert[2].mFactorC;
				int right_top_v2    = tri->mVert[2].mFactorA * xright + tri->mVert[2].mFactorB * ytop    + tri->mVert[2].mFactorC;

				const bool positive_half0  = (left_bottom_v0  >= 0) &&
											 (left_top_v0     >= 0) &&
											 (right_bottom_v0 >= 0) &&
											 (right_top_v0    >= 0);
				const bool negative_half0  = (left_bottom_v0  <  0) &&
											 (left_top_v0     <  0) &&
											 (right_bottom_v0 <  0) &&
											 (right_top_v0    <  0);
				const bool positive_half1  = (left_bottom_v1  >= 0) &&
											 (left_top_v1     >= 0) &&
											 (right_bottom_v1 >= 0) &&
											 (right_top_v1    >= 0);
				const bool negative_half1  = (left_bottom_v1  <  0) &&
											 (left_top_v1     <  0) &&
											 (right_bottom_v1 <  0) &&
											 (right_top_v1    <  0);
				const bool positive_half2  = (left_bottom_v2  >= 0) &&
											 (left_top_v2     >= 0) &&
											 (right_bottom_v2 >= 0) &&
											 (right_top_v2    >= 0);
				const bool negative_half2  = (left_bottom_v2  <  0) &&
											 (left_top_v2     <  0) &&
											 (right_bottom_v2 <  0) &&
											 (right_top_v2    <  0);

				// Micro tile is totally outside the triangle
				if (negative_half0 || negative_half1 || negative_half2)
				{
					continue;
				}
				else
				{
					float z = tri->mZAtOrigin + tri->mZGradientX * xp + tri->mZGradientY * yp;

					// Micro tile is totally inside the triangle
					if (positive_half0 && positive_half1 && positive_half2)
					{
						for (int k = 0; k < MICRO_TILE_SIZE && (yp + k) < g_GC->mRT.height; ++k)
						{
							float zx = z;

							for (int l = 0; l < MICRO_TILE_SIZE && (xp + l) < g_GC->mRT.width; ++l)
							{
								const int xoff = (j << MICRO_TILE_SIZE_SHIFT) + l;
								const int yoff = (i << MICRO_TILE_SIZE_SHIFT) + k;

								if (zx < z_buf[yoff][xoff])
								{
									z_buf [yoff][xoff] = zx;
									pp_map[yoff][xoff] = tri;
								}

								zx += tri->mZGradientX;
							}
							z += tri->mZGradientY;
						}
					}
					// This micro tile totally contain the triangle(can do OPT ?),
					// or it's clipped against the triangle edges, so need further rasterization
					else
					{
						for (int k = 0; k < MICRO_TILE_SIZE && (yp + k) < g_GC->mRT.height; ++k)
						{
							int sum0 = left_bottom_v0;
							int sum1 = left_bottom_v1;
							int sum2 = left_bottom_v2;

							for (int l = 0; l < MICRO_TILE_SIZE && (xp + l) < g_GC->mRT.width; ++l)
							{
								if (sum0 > 0 && sum1 > 0 && sum2 > 0)
								{
									const int xoff = (j << MICRO_TILE_SIZE_SHIFT) + l;
									const int yoff = (i << MICRO_TILE_SIZE_SHIFT) + k;

									const float zp = z + l * tri->mZGradientX + k * tri->mZGradientY;
									if (zp < z_buf[yoff][xoff])
									{
										z_buf [yoff][xoff] = zp;
										pp_map[yoff][xoff] = tri;
									}
								}
								sum0 += tri->mVert[0].mFactorA;
								sum1 += tri->mVert[1].mFactorA;
								sum2 += tri->mVert[2].mFactorA;
							}

							left_bottom_v0 += tri->mVert[0].mFactorB;
							left_bottom_v1 += tri->mVert[1].mFactorB;
							left_bottom_v2 += tri->mVert[2].mFactorB;
						}
					}
				}
			}
		}
	}

	// TODO: early z/stencil
	// TODO: hierarcical z/stencil
#if 0
	for (int i = 0; i < MACRO_TILE_SIZE; ++i)
	{
		for (int j = 0; j < MACRO_TILE_SIZE; ++j)
		{
			const int &index = (g_GC->mRT.height - (y + i) - 1) * g_GC->mRT.width + x + j;

			if (pp_map[i][j] && z_buf[i][j] < g_GC->mRT.pDepthBuffer[index])
			{
				RenderOnePixel(pp_map[i][j], x + j, y + i, z_buf[i][j]);
			}
		}
	}
#endif

	if (!mFlushTriggerBySwapBuffer)
	{
		// depth buffer store.
	}

#if 1
	for (int i = 0; i < MACRO_TILE_SIZE && (y + i) < (g_GC->mRT.height - 1); i += 2)
	{
		for (int j = 0; j < MACRO_TILE_SIZE && (x + j) < (g_GC->mRT.width - 1); j += 2)
		{
			RenderQuadPixels(pp_map, x, y, i, j);
		}
	}
#endif

}

void TBDR::RenderOnePixel(Triangle *tri, int x, int y, float z)
{
	Fsio fsio;
	fsio.x = x;
	fsio.y = y;
	fsio.z = z;
	fsio.mIndex  = (g_GC->mRT.height - y - 1) * g_GC->mRT.width + x;
	fsio.m_priv0 = tri;
	fsio.in.resize(tri->mPrim.mVert[0].getRegsNum());

	// TODO: Add condition check
	mDE.mInterpolater->emit(&fsio);
}

void TBDR::RenderQuadPixels(PixelPrimMap pp_map, int x, int y, int i, int j)
{
	int quad_mask = 0xf;

	Triangle *tri[4] =
	{
		pp_map[i    ][j    ],
		pp_map[i    ][j + 1],
		pp_map[i + 1][j    ],
		pp_map[i + 1][j + 1]
	};

	for (int s = 0; s < 4; s++)
	{
		if (tri[s] == nullptr)
			quad_mask &= ~(1 << s);
	}

	if (!quad_mask)
		return;

	unsigned long idx;
	while(_BitScanForward(&idx, (unsigned long)quad_mask))
	{
		int coverage_mask = 0;
		for (int s = 0; s < 4; s++)
		{
			if (tri[s] == tri[idx])
				coverage_mask = 1 << idx;
		}

		quad_mask &= ~coverage_mask;

		RenderQuadPixelsInOneTriangle(tri[idx], coverage_mask, x, y, i, j);
	}
}

void TBDR::RenderQuadPixelsInOneTriangle(Triangle *tri, int coverage_mask, int x, int y, int i, int j)
{
	/* NOTE:
	 * Put fsio at stack to optimize memory allocation.
	 * However, since Fsiosimd is a large structure containing the shader input/output registers,
	 * need take care if the max register number is increased.
	 */
	ALIGN(16) Fsiosimd fsio;

	// TODO: need get FS from primitive state
	FragmentShader *pFS = g_GC->mPM.getCurrentProgram()->getFS();
	size_t fsin_num     = tri->mPrim.mVert[0].getRegsNum();
	size_t fsout_num    = pFS->getOutRegsNum();

	fsio.mInRegsNum  = fsin_num;
	fsio.mOutRegsNum = fsout_num;
	fsio.x = x + j;
	fsio.y = y + i;
	fsio.mCoverageMask = coverage_mask;
	fsio.m_priv0 = tri;

	// TODO: elaborate the pipe stages order
	mDE.mInterpolater->emit(&fsio);

	FragmentShader *fs = tri->mPrim.mDC->mFS;
	fs->emit(&fsio);
	mDE.mFBWriter->emit(&fsio);
}

void TBDR::finalize()
{
	::glsp::ThreadPool::get().waitForAllTaskDone();

	MemoryPoolMT::get().BoostReclaimAll();

	for (int y = 0; y < MAX_TILES_IN_HEIGHT; ++y)
	{
		for (int x = 0; x < MAX_TILES_IN_WIDTH; ++x)
		{
			std::vector<Triangle *> &disp_list = s_DispList[y][x];
			std::vector<Triangle *> &full_disp_list = s_FullCoverDispList[y][x];

			disp_list.clear();
			full_disp_list.clear();
		}
	}

	if (mDepthClearFlag)
		mDepthClearFlag = false;
}

void TBDR::FlushDisplayLists(bool swap_buffer)
{
	mFlushTriggerBySwapBuffer = swap_buffer;

	onRasterizing();

	finalize();
}

} // namespace glsp
