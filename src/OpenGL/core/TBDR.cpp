// Avoid MSVC to see min max as macros rather than std functions
#define NOMINMAX 1

#include "TBDR.h"

#include <algorithm>
#include <cstring>

#include "ThreadPool.h"
#include "MemoryPool.h"
#include "GLContext.h"
#include "DrawEngine.h"
#include "glsp_spinlock.h"
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
	for (Primitive *prim: bat->mPrims)
	{
		Triangle *tri = new(MemoryPoolMT::get()) Triangle(*prim);

		const int xmin = ROUND_DOWN(tri->xmin, MACRO_TILE_SIZE);
		const int ymin = ROUND_DOWN(tri->ymin, MACRO_TILE_SIZE);

		for (int y = ymin; y <= tri->ymax; y += MACRO_TILE_SIZE)
		{
			for (int x = xmin; x <= tri->xmax; x += MACRO_TILE_SIZE)
			{
				const int & xleft   = x;
				const int & ybottom = y;
				const int & xright  = x + MACRO_TILE_SIZE - 1;
				const int & ytop    = y + MACRO_TILE_SIZE - 1;

				const int left_bottom_v0  = tri->mVert[0].mFactorA * xleft  + tri->mVert[0].mFactorB * ybottom + tri->mVert[0].mFactorC;
				const int left_top_v0     = tri->mVert[0].mFactorA * xleft  + tri->mVert[0].mFactorB * ytop    + tri->mVert[0].mFactorC;
				const int right_bottom_v0 = tri->mVert[0].mFactorA * xright + tri->mVert[0].mFactorB * ybottom + tri->mVert[0].mFactorC;
				const int right_top_v0    = tri->mVert[0].mFactorA * xright + tri->mVert[0].mFactorB * ytop    + tri->mVert[0].mFactorC;

				const int left_bottom_v1  = tri->mVert[1].mFactorA * xleft  + tri->mVert[1].mFactorB * ybottom + tri->mVert[1].mFactorC;
				const int left_top_v1     = tri->mVert[1].mFactorA * xleft  + tri->mVert[1].mFactorB * ytop    + tri->mVert[1].mFactorC;
				const int right_bottom_v1 = tri->mVert[1].mFactorA * xright + tri->mVert[1].mFactorB * ybottom + tri->mVert[1].mFactorC;
				const int right_top_v1    = tri->mVert[1].mFactorA * xright + tri->mVert[1].mFactorB * ytop    + tri->mVert[1].mFactorC;

				const int left_bottom_v2  = tri->mVert[2].mFactorA * xleft  + tri->mVert[2].mFactorB * ybottom + tri->mVert[2].mFactorC;
				const int left_top_v2     = tri->mVert[2].mFactorA * xleft  + tri->mVert[2].mFactorB * ytop    + tri->mVert[2].mFactorC;
				const int right_bottom_v2 = tri->mVert[2].mFactorA * xright + tri->mVert[2].mFactorB * ybottom + tri->mVert[2].mFactorC;
				const int right_top_v2    = tri->mVert[2].mFactorA * xright + tri->mVert[2].mFactorB * ytop    + tri->mVert[2].mFactorC;

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
				// This macro tile is totally outside the triangle
				if (negative_half0 || negative_half1 || negative_half2)
				{
					continue;
				}
				else
				{
					// This macro tile is totally inside the triangle
					if (positive_half0 && positive_half1 && positive_half2)
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
	getNextStage()->emit(data);
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

	//float __attribute__((aligned(16))) pcbc0[4];
	//float __attribute__((aligned(16))) pcbc1[4];
	//float __attribute__((aligned(16))) w[4];
	//_mm_store_ps(pcbc0, vPCBC0);
	//_mm_store_ps(pcbc1, vPCBC1);
	//_mm_store_ps(w, vW);

	//printf("jzb: pcbc0: %f %f %f %f\n", pcbc0[0], pcbc0[1], pcbc0[2], pcbc0[3]);
	//printf("jzb: pcbc1: %f %f %f %f\n", pcbc1[0], pcbc1[1], pcbc1[2], pcbc1[3]);
	//printf("jzb: w: %f %f %f %f\n", w[0], w[1], w[2], w[3]);
	//printf("jzb: original w: %f %f %f\n",
			//tri->mPrim.mVert[0].position().w,
			//tri->mPrim.mVert[1].position().w,
			//tri->mPrim.mVert[2].position().w);

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

	mVert2 = v2;

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
	mVert[0].mFactorA = y1y2 << RAST_SUBPIXEL_BITS;
	mVert[0].mFactorB = x2x1 << RAST_SUBPIXEL_BITS;
	mVert[0].mFactorC = C0 + (y1y2 << (RAST_SUBPIXEL_BITS - 1)) + (x2x1 << (RAST_SUBPIXEL_BITS - 1));

	mVert[1].mFactorA = y2y0 << RAST_SUBPIXEL_BITS;
	mVert[1].mFactorB = x0x2 << RAST_SUBPIXEL_BITS;
	mVert[1].mFactorC = C1 + (y2y0 << (RAST_SUBPIXEL_BITS - 1)) + (x0x2 << (RAST_SUBPIXEL_BITS - 1));

	mVert[2].mFactorA = y0y1 << RAST_SUBPIXEL_BITS;
	mVert[2].mFactorB = x1x0 << RAST_SUBPIXEL_BITS;
	mVert[2].mFactorC = C2 + (y0y1 << (RAST_SUBPIXEL_BITS - 1)) + (x1x0 << (RAST_SUBPIXEL_BITS - 1));

	xmin = (std::min({x0, x1, x2}) >> RAST_SUBPIXEL_BITS);
	ymin = (std::min({y0, y1, y2}) >> RAST_SUBPIXEL_BITS);
	xmax = ((ROUND_UP(std::max({x0, x1, x2}), RAST_SUBPIXELS) >> RAST_SUBPIXEL_BITS) - 1);
	ymax = ((ROUND_UP(std::max({y0, y1, y2}), RAST_SUBPIXELS) >> RAST_SUBPIXEL_BITS) - 1);

	xmin = clamp(xmin, 0, g_GC->mRT.width  - 1);
	xmax = clamp(xmax, 0, g_GC->mRT.width  - 1);
	ymin = clamp(ymin, 0, g_GC->mRT.height - 1);
	ymax = clamp(ymax, 0, g_GC->mRT.height - 1);

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

	mPCBCOnW0GradientX = lambdax0;
	mPCBCOnW0GradientY = lambday0;
	mPCBCOnW0AtOrigin  = WReciprocal0 - mPCBCOnW0GradientX * xoffset - mPCBCOnW0GradientY * yoffset;

	mPCBCOnW1GradientX = lambdax1;
	mPCBCOnW1GradientY = lambday1;
	mPCBCOnW1AtOrigin  = -mPCBCOnW1GradientX * xoffset - mPCBCOnW1GradientY * yoffset;

	mWRecipGradientX   = lambdax0 + lambdax1 + lambdax2;
	mWRecipGradientY   = lambday0 + lambday1 + lambday2;
	mWRecipAtOrigin    = WReciprocal0 - mWRecipGradientX * xoffset - mWRecipGradientY * yoffset;

	mZGradientX = y1y2f * v0->position().z + y2y0f * v1->position().z + y0y1f * v2->position().z;
	mZGradientY = x2x1f * v0->position().z + x0x2f * v1->position().z + x1x0f * v2->position().z;
	mZAtOrigin  = v0->position().z - mZGradientX * xoffset - mZGradientY * yoffset;

	const size_t size = v0->getRegsNum();
	mAttrPlaneEquationA.resize(size);
	mAttrPlaneEquationB.resize(size);

	for (size_t i = 1; i < size; ++i)
	{
		mAttrPlaneEquationA.getReg(i) = v0->getReg(i) - v2->getReg(i);
		mAttrPlaneEquationB.getReg(i) = v1->getReg(i) - v2->getReg(i);
	}
#if 0
	const int texCoordLoc = g_GC->mPM.getCurrentProgram()->getFS()->GetTextureCoordLocation();

	{
		const float &dudx = mGradientX[texCoordLoc].x;
		const float &dvdx = mGradientX[texCoordLoc].y;
		const float &dudy = mGradientY[texCoordLoc].x;
		const float &dvdy = mGradientY[texCoordLoc].y;
		const float &dzdx = mGradientX[0].w;
		const float &dzdy = mGradientY[0].w;
		const float &z0   = mVert[0].mAttrReciprocal[0].w;
		const float &u0   = mVert[0].mAttrReciprocal[texCoordLoc].x;
		const float &v0   = mVert[0].mAttrReciprocal[texCoordLoc].y;

		a = dudx * dzdy - dzdx * dudy;
		b = dvdx * dzdy - dzdx * dvdy;
		c = dudx * z0   - dzdx * u0;
		d = dvdx * z0   - dzdx * v0;
		e = dudy * z0   - dzdy * u0;
		f = dvdy * z0   - dzdy * v0;
	}
#endif
}

TBDR::TBDR():
	Rasterizer()
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

void TBDR::onRasterizing(DrawContext *dc)
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

	for (int i = 0; i < MACRO_TILE_SIZE; ++i)
	{
		for (int j = 0; j < MACRO_TILE_SIZE; ++j)
		{
			z_buf[i][j] = 1.0f;
		}
	}
	std::memset(&pp_map[0][0], 0, sizeof(PixelPrimMap));

	for (Triangle *tri: full_disp_list)
	{
		float z = tri->mZAtOrigin + tri->mZGradientX * x + tri->mZGradientY * y;

		for (int i = 0; i < MACRO_TILE_SIZE; ++i)
		{
			float zx = z;

			for (int j = 0; j < MACRO_TILE_SIZE; ++j)
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
		for (int yp = y, i = 0; i < MICRO_TILES_IN_MACRO_TILE; yp += MICRO_TILE_SIZE, ++i)
		{
			for (int xp = x, j = 0; j < MICRO_TILES_IN_MACRO_TILE; xp += MICRO_TILE_SIZE, ++j)
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
						for (int k = 0; k < MICRO_TILE_SIZE; ++k)
						{
							float zx = z;

							for (int l = 0; l < MICRO_TILE_SIZE; ++l)
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
						for (int k = 0; k < MICRO_TILE_SIZE; ++k)
						{
							int sum0 = left_bottom_v0;
							int sum1 = left_bottom_v1;
							int sum2 = left_bottom_v2;

							for (int l = 0; l < MICRO_TILE_SIZE; ++l)
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
#if 1
	for (int i = 0; i < MACRO_TILE_SIZE; i += 2)
	{
		for (int j = 0; j < MACRO_TILE_SIZE; j += 2)
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

	getNextStage()->emit(&fsio);
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

	getNextStage()->emit(&fsio);
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
}

} // namespace glsp
