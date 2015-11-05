#include "TBDR.h"

#include <algorithm>
#include <cstring>

#include "ThreadPool.h"
#include "MemoryPool.h"
#include "GLContext.h"
#include "DrawEngine.h"
#include "utils.h"
#include "common/glsp_spinlock.h"


NS_OPEN_GLSP_OGL()

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
				// FIXME: need consider the conversion from pixel coords to fixed-point coords
				// But so far, looks OK.
				const int xleft   = x << FIXED_POINT4_SHIFT;
				const int ybottom = y << FIXED_POINT4_SHIFT;
				const int xright  = (x + MACRO_TILE_SIZE - 1) << FIXED_POINT4_SHIFT;
				const int ytop    = (y + MACRO_TILE_SIZE - 1) << FIXED_POINT4_SHIFT;

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

				const bool positive_half0  = (left_bottom_v0  >  0) &&
											 (left_top_v0     >  0) &&
											 (right_bottom_v0 >  0) &&
											 (right_top_v0    >  0);
				const bool negative_half0  = (left_bottom_v0  <= 0) &&
											 (left_top_v0     <= 0) &&
											 (right_bottom_v0 <= 0) &&
											 (right_top_v0    <= 0);
				const bool positive_half1  = (left_bottom_v1  >  0) &&
											 (left_top_v1     >  0) &&
											 (right_bottom_v1 >  0) &&
											 (right_top_v1    >  0);
				const bool negative_half1  = (left_bottom_v1  <= 0) &&
											 (left_top_v1     <= 0) &&
											 (right_bottom_v1 <= 0) &&
											 (right_top_v1    <= 0);
				const bool positive_half2  = (left_bottom_v2  >  0) &&
											 (left_top_v2     >  0) &&
											 (right_bottom_v2 >  0) &&
											 (right_top_v2    >  0);
				const bool negative_half2  = (left_bottom_v2  <= 0) &&
											 (left_top_v2     <= 0) &&
											 (right_bottom_v2 <= 0) &&
											 (right_top_v2    <= 0);
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

void PerspectiveCorrectInterpolater::emit(void *data)
{
	Fsio &fsio = *static_cast<Fsio *>(data);
	const Triangle *tri = static_cast<Triangle *>(fsio.m_priv0);
	const fsInput &base = tri->mVert[0].mAttrReciprocal;

	onInterpolating(base,
					tri->mGradientX,
					tri->mGradientY,
					fsio.x + 0.5f - base.position().x,
					fsio.y + 0.5f - base.position().y,
					fsio.in);

	getNextStage()->emit(data);
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

Triangle::Triangle(Primitive &prim):
	mPrim(prim)
{
	vsOutput *v0;
	vsOutput *v1;
	vsOutput *v2;

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

	const int x0 = fixedpoint_cast<FIXED_POINT4>(v0->position().x);
	const int y0 = fixedpoint_cast<FIXED_POINT4>(v0->position().y);

	const int x1 = fixedpoint_cast<FIXED_POINT4>(v1->position().x);
	const int y1 = fixedpoint_cast<FIXED_POINT4>(v1->position().y);

	const int x2 = fixedpoint_cast<FIXED_POINT4>(v2->position().x);
	const int y2 = fixedpoint_cast<FIXED_POINT4>(v2->position().y);

	const float ZReciprocal0 = 1.0f / v0->position().w;
	const float ZReciprocal1 = 1.0f / v1->position().w;
	const float ZReciprocal2 = 1.0f / v2->position().w;

	const size_t size = v0->getRegsNum();
	mVert[0].mAttrReciprocal.resize(size);
	mVert[1].mAttrReciprocal.resize(size);
	mVert[2].mAttrReciprocal.resize(size);

	mVert[0].mAttrReciprocal.position().x = v0->position().x;
	mVert[0].mAttrReciprocal.position().y = v0->position().y;
	mVert[0].mAttrReciprocal.position().z = v0->position().z;
	mVert[0].mAttrReciprocal.position().w = ZReciprocal0;

	mVert[1].mAttrReciprocal.position().x = v1->position().x;
	mVert[1].mAttrReciprocal.position().y = v1->position().y;
	mVert[1].mAttrReciprocal.position().z = v1->position().z;
	mVert[1].mAttrReciprocal.position().w = ZReciprocal1;

	mVert[2].mAttrReciprocal.position().x = v2->position().x;
	mVert[2].mAttrReciprocal.position().y = v2->position().y;
	mVert[2].mAttrReciprocal.position().z = v2->position().z;
	mVert[2].mAttrReciprocal.position().w = ZReciprocal2;

	for (size_t i = 1; i < v0->getRegsNum(); ++i)
	{
		mVert[0].mAttrReciprocal.getReg(i) = v0->getReg(i) * ZReciprocal0;
		mVert[1].mAttrReciprocal.getReg(i) = v1->getReg(i) * ZReciprocal1;
		mVert[2].mAttrReciprocal.getReg(i) = v2->getReg(i) * ZReciprocal2;
	}

	const int x1x0 = x1 - x0;
	const int x2x1 = x2 - x1;
	const int x0x2 = x0 - x2;

	const int y0y1 = y0 - y1;
	const int y1y2 = y1 - y2;
	const int y2y0 = y2 - y0;

	int C0 = x1 * y2 - x2 * y1;
	int C1 = x2 * y0 - x0 * y2;
	int C2 = x0 * y1 - x1 * y0;

	if (y0y1 > 0 || (y0y1 == 0 && x1x0 < 0))  C2++;
	if (y1y2 > 0 || (y1y2 == 0 && x2x1 < 0))  C0++;
	if (y2y0 > 0 || (y2y0 == 0 && x0x2 < 0))  C1++;

	mVert[0].mFactorA = y1y2;
	mVert[0].mFactorB = x2x1;
	mVert[0].mFactorC = C0;

	mVert[1].mFactorA = y2y0;
	mVert[1].mFactorB = x0x2;
	mVert[1].mFactorC = C1;

	mVert[2].mFactorA = y0y1;
	mVert[2].mFactorB = x1x0;
	mVert[2].mFactorC = C2;

	xmin = (std::min({x0, x1, x2}) >> FIXED_POINT4_SHIFT);
	ymin = (std::min({y0, y1, y2}) >> FIXED_POINT4_SHIFT);
	xmax = (ROUND_UP(std::max({x0, x1, x2}), FIXED_POINT4) >> FIXED_POINT4_SHIFT);
	ymax = (ROUND_UP(std::max({y0, y1, y2}), FIXED_POINT4) >> FIXED_POINT4_SHIFT);

	xmin = clamp(xmin, 0, g_GC->mRT.width  - 1);
	xmax = clamp(xmax, 0, g_GC->mRT.width  - 1);
	ymin = clamp(ymin, 0, g_GC->mRT.height - 1);
	ymax = clamp(ymax, 0, g_GC->mRT.height - 1);

	mGradientX.resize(size);
	mGradientY.resize(size);

	const float y1y2f = (v1->position().y - v2->position().y) * prim.mAreaReciprocal;
	const float y2y0f = (v2->position().y - v0->position().y) * prim.mAreaReciprocal;
	const float y0y1f = (v0->position().y - v1->position().y) * prim.mAreaReciprocal;

	const float x2x1f = (v2->position().x - v1->position().x) * prim.mAreaReciprocal;
	const float x0x2f = (v0->position().x - v2->position().x) * prim.mAreaReciprocal;
	const float x1x0f = (v1->position().x - v0->position().x) * prim.mAreaReciprocal;

#define GRADIENCE_EQUATION(i, c)	\
	mGradientX[i].c = y1y2f * mVert[0].mAttrReciprocal[i].c +	\
					  y2y0f * mVert[1].mAttrReciprocal[i].c +	\
					  y0y1f * mVert[2].mAttrReciprocal[i].c;		\
	mGradientY[i].c = x2x1f * mVert[0].mAttrReciprocal[i].c +	\
					  x0x2f * mVert[1].mAttrReciprocal[i].c +	\
					  x1x0f * mVert[2].mAttrReciprocal[i].c;

	mGradientX[0].x = 1.0f;
	mGradientX[0].y = 0.0f;

	mGradientY[0].x = 0.0f;
	mGradientY[0].y = 1.0f;

	GRADIENCE_EQUATION(0, z);
	GRADIENCE_EQUATION(0, w);

	for(size_t i = 1; i < size; i++)
	{
		GRADIENCE_EQUATION(i, x);
		GRADIENCE_EQUATION(i, y);
		GRADIENCE_EQUATION(i, z);
		GRADIENCE_EQUATION(i, w);
	}
#undef GRADIENCE_EQUATION

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
		float z = tri->mVert[0].mAttrReciprocal.position().z +
				  tri->mGradientX[0].z * (x + 0.5f - tri->mVert[0].mAttrReciprocal.position().x) +
				  tri->mGradientY[0].z * (y + 0.5f - tri->mVert[0].mAttrReciprocal.position().y);

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

				zx += tri->mGradientX[0].z;
			}
			z += tri->mGradientY[0].z;
		}
	}

	for (Triangle *tri: disp_list)
	{
#if 0
		const float x0 = tri->mVert[0].mAttrReciprocal.position().x;
		const float y0 = tri->mVert[0].mAttrReciprocal.position().y;

		const float x1 = tri->mVert[1].mAttrReciprocal.position().x;
		const float y1 = tri->mVert[1].mAttrReciprocal.position().y;

		const float x2 = tri->mVert[2].mAttrReciprocal.position().x;
		const float y2 = tri->mVert[2].mAttrReciprocal.position().y;

		const float x1x0 = x1 - x0;
		const float x2x1 = x2 - x1;
		const float x0x2 = x0 - x2;

		const float y0y1 = y0 - y1;
		const float y1y2 = y1 - y2;
		const float y2y0 = y2 - y0;

		float C0 = x1 * y2 - x2 * y1;
		float C1 = x2 * y0 - x0 * y2;
		float C2 = x0 * y1 - x1 * y0;
		float z = tri->mVert[0].mAttrReciprocal.position().z +
				  tri->mGradientX[0].z * (x + 0.5f - tri->mVert[0].mAttrReciprocal.position().x) +
				  tri->mGradientY[0].z * (y + 0.5f - tri->mVert[0].mAttrReciprocal.position().y);

		for (int yp = y, i = 0; i < MACRO_TILE_SIZE; yp++, ++i)
		{
			for (int xp = x, j = 0; j < MACRO_TILE_SIZE; xp++, ++j)
			{
				float sum0  = y0y1 * (xp + 0.5f) + x1x0 * (yp + 0.5f) + C2;

				float sum1  = y1y2 * (xp + 0.5f) + x2x1 * (yp + 0.5f) + C0;

				float sum2  = y2y0 * (xp + 0.5f) + x0x2 * (yp + 0.5f) + C1;
								if (sum0 >= 0.0f && sum1 >= 0.0f && sum2 >= 0.0f)
								{
									const int xoff = j;
									const int yoff = i;

									const float zp = z + j * tri->mGradientX[0].z + i * tri->mGradientY[0].z;
									if (zp < z_buf[yoff][xoff])
									{
										z_buf [yoff][xoff] = zp;
										pp_map[yoff][xoff] = tri;
									}
								}
								else
								{
								}
			}
		}
#endif
#if 1
		const int A0 = tri->mVert[0].mFactorA << FIXED_POINT4_SHIFT;
		const int A1 = tri->mVert[1].mFactorA << FIXED_POINT4_SHIFT;
		const int A2 = tri->mVert[2].mFactorA << FIXED_POINT4_SHIFT;
		const int B0 = tri->mVert[0].mFactorB << FIXED_POINT4_SHIFT;
		const int B1 = tri->mVert[1].mFactorB << FIXED_POINT4_SHIFT;
		const int B2 = tri->mVert[2].mFactorB << FIXED_POINT4_SHIFT;

		for (int yp = y, i = 0; i < MICRO_TILES_IN_MACRO_TILE; yp += MICRO_TILE_SIZE, ++i)
		{
			for (int xp = x, j = 0; j < MICRO_TILES_IN_MACRO_TILE; xp += MICRO_TILE_SIZE, ++j)
			{
				//const int xleft   = (xp << FIXED_POINT4_SHIFT) + FIXED_POINT4 / 2;
				//const int ybottom = (yp << FIXED_POINT4_SHIFT) + FIXED_POINT4 / 2;
				//const int xright  = ((xp + MICRO_TILE_SIZE - 1) << FIXED_POINT4_SHIFT) + FIXED_POINT4 / 2;
				//const int ytop    = ((yp + MICRO_TILE_SIZE - 1) << FIXED_POINT4_SHIFT) + FIXED_POINT4 / 2;

				const int xleft   = (xp << FIXED_POINT4_SHIFT);
				const int ybottom = (yp << FIXED_POINT4_SHIFT);
				const int xright  = ((xp + MICRO_TILE_SIZE - 1) << FIXED_POINT4_SHIFT);
				const int ytop    = ((yp + MICRO_TILE_SIZE - 1) << FIXED_POINT4_SHIFT);

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

				const bool positive_half0  = (left_bottom_v0  >  0) &&
											 (left_top_v0     >  0) &&
											 (right_bottom_v0 >  0) &&
											 (right_top_v0    >  0);
				const bool negative_half0  = (left_bottom_v0  <= 0) &&
											 (left_top_v0     <= 0) &&
											 (right_bottom_v0 <= 0) &&
											 (right_top_v0    <= 0);
				const bool positive_half1  = (left_bottom_v1  >  0) &&
											 (left_top_v1     >  0) &&
											 (right_bottom_v1 >  0) &&
											 (right_top_v1    >  0);
				const bool negative_half1  = (left_bottom_v1  <= 0) &&
											 (left_top_v1     <= 0) &&
											 (right_bottom_v1 <= 0) &&
											 (right_top_v1    <= 0);
				const bool positive_half2  = (left_bottom_v2  >  0) &&
											 (left_top_v2     >  0) &&
											 (right_bottom_v2 >  0) &&
											 (right_top_v2    >  0);
				const bool negative_half2  = (left_bottom_v2  <= 0) &&
											 (left_top_v2     <= 0) &&
											 (right_bottom_v2 <= 0) &&
											 (right_top_v2    <= 0);

				// Micro tile is totally outside the triangle
				if (negative_half0 || negative_half1 || negative_half2)
				{
					continue;
				}
				else
				{
					float z = tri->mVert[0].mAttrReciprocal.position().z +
							  tri->mGradientX[0].z * (xp + 0.5f - tri->mVert[0].mAttrReciprocal.position().x) +
							  tri->mGradientY[0].z * (yp + 0.5f - tri->mVert[0].mAttrReciprocal.position().y);

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

								zx += tri->mGradientX[0].z;
							}
							z += tri->mGradientY[0].z;
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

									const float zp = z + l * tri->mGradientX[0].z + k * tri->mGradientY[0].z;
									if (zp < z_buf[yoff][xoff])
									{
										z_buf [yoff][xoff] = zp;
										pp_map[yoff][xoff] = tri;
									}
								}
								sum0 += A0;
								sum1 += A1;
								sum2 += A2;
							}

							left_bottom_v0 += B0;
							left_bottom_v1 += B1;
							left_bottom_v2 += B2;
						}
					}
				}
			}
		}
#endif
	}

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

NS_CLOSE_GLSP_OGL()
