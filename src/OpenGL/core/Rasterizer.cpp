#include "Rasterizer.h"

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <list>
#include <thread>
#include <unordered_map>
#include <vector>

#include "DrawEngine.h"
#include "GLContext.h"
#include "PixelBackend.h"
#include "utils.h"
#include "ThreadPool.h"
#include "common/glsp_debug.h"


NS_OPEN_GLSP_OGL()

using glm::vec3;
using glm::vec4;

const RenderTarget *gRT = NULL;

class PerspectiveCorrectInterpolater: public Interpolater
{
public:
	PerspectiveCorrectInterpolater() = default;
	~PerspectiveCorrectInterpolater() = default;

	// PipeStage interface
	virtual void emit(void *data);

	// Should be invoked before onInterpolating()
	virtual void CalculateRadiences(Gradience *pGrad);

	void onInterpolating(const float in,
				 		const float &gradX,
				 		const float &gradY,
				 		float stepX, float stepY,
				 		float &out);

	virtual void onInterpolating(const fsInput &in,
								 const fsInput &gradX,
								 const fsInput &gradY,
								 float stepX, float stepY,
								 fsInput &out);

	// step 1 in X or Y axis
	virtual void onInterpolating(const fsInput &in,
								 const fsInput &grad,
								 float x,
								 fsInput &out);

private:
	static inline void PerspectiveCorrect(fsInput &in);
};


// Fragment shader hook
class FSHook: public PipeStage
{
public:
	FSHook():
		PipeStage("FragmentShader hook", DrawEngine::getDrawEngine())
	{
	}

	~FSHook() = default;

	virtual void emit(void *data);
	virtual void finalize() { }

private:

};


// active edge table implementation
// TODO: tile-based implementation
class ScanlineRasterizer: public Rasterizer
{
public:
	friend class RasterizerWrapper;
	class span;
	class edge;
	class triangle;
	class SRHelper;

	typedef std::unordered_map<int, std::vector<edge *> > GlobalEdgeTableY;
	typedef std::list<edge *> ActiveEdgeTableY;
	typedef std::list<span *> ActiveEdgeTableX;

	ScanlineRasterizer():
		Rasterizer(),
		pfnHSR(HiddenSurfaceRemoval),
		mFShook(),
		mBlockPool(::glsp::ThreadPool::get().getThreadsNumber() / 2  +
			::glsp::ThreadPool::get().getThreadsNumber())
	{
	}

	virtual ~ScanlineRasterizer() = default;

	PipeStage* getFShook() { return &mFShook; }

protected:
	virtual void onRasterizing(DrawContext *dc);

private:
	// Associate with the task dispatch
	class LargeBlockPoolForTaskDispatch
	{
	public:
		LargeBlockPoolForTaskDispatch(int slots);
		~LargeBlockPoolForTaskDispatch();

		void* allocate(int index, size_t size);
		void deallocate(int index);

	private:
		struct PoolMetaData {
			void 		 *mStorage;
			unsigned int  mSize;
			volatile bool mbInUse;
		};

		PoolMetaData *mMetaData;
		const int     mSlotsNum;
	};

	SRHelper* createGET(DrawContext *dc);
	void scanConversion(SRHelper *hlp);
	void activateEdgesFromGET(SRHelper *hlp, int y);
	void removeEdgeFromAET(SRHelper *hlp, int y);

	// perspective-correct interpolation
	void interpolate(glm::vec3 &coeff, Primitive& prim, fsInput& result);
	void traversalAET(SRHelper *hlp, int y);
	void advanceEdgesInAET(SRHelper *hlp);

	void RenderPointFromOneSpan(span &sp, int x, int y);

	void finalize(SRHelper *hlp);

	static void HiddenSurfaceRemoval(ScanlineRasterizer *pSR, const ActiveEdgeTableX &aetx, int x, int y);
	static void HiddenSurfaceRemovalTranslucent(ScanlineRasterizer *pSR, const ActiveEdgeTableX &aetx, int x, int y);

	void (*pfnHSR)(ScanlineRasterizer *pSR, const ActiveEdgeTableX &aetx, int x, int y);
	FSHook mFShook;

	LargeBlockPoolForTaskDispatch mBlockPool;
};


class ScanlineRasterizer::triangle
{
public:
	triangle(const Primitive &prim):
		mGrad(prim),
		mPrim(prim),
		mActiveEdge0(NULL),
		mActiveEdge1(NULL)
	{
	}
	~triangle() = default;

	vec3 calculateBC(float xp, float yp);
	void setActiveEdge(edge *pEdge);
	void unsetActiveEdge(edge *pEdge);
	edge* getAdjcentEdge(edge *pEdge);

	Gradience mGrad;
	const Primitive &mPrim;

private:
	// There can be at most two edges in AET at the same time
	edge *mActiveEdge0;
	edge *mActiveEdge1;
};


class ScanlineRasterizer::edge
{
public:
	edge(triangle *pParent):
		mParent(pParent),
		bActive(false)
	{
	}
	~edge() = default;

	float x;
	float dx;
	int ymax;
	triangle *mParent;
	bool bActive;
};


class ScanlineRasterizer::span
{
public:
	span(int x1, int x2, triangle *pParent):
		xleft(x1),
		xright(x2),
		mParent(pParent),
		bInterpolated(false)
	{
	}
	~span() = default;

	int xleft;
	int xright;
	float mCurrentZ;
	triangle *mParent;
	bool bInterpolated;

	fsInput mStart;
};


class ScanlineRasterizer::SRHelper
{
public:
	SRHelper() = default;
	~SRHelper() = default;

	int ymin;
	int ymax;
	std::vector<triangle *> mTri;

	// global edges table
	GlobalEdgeTableY mGET;
	// active edges table
	ActiveEdgeTableY mAET;
};


ScanlineRasterizer::LargeBlockPoolForTaskDispatch::LargeBlockPoolForTaskDispatch(int slots):
	mSlotsNum(slots)
{
	mMetaData = static_cast<PoolMetaData *>(calloc(sizeof(PoolMetaData), mSlotsNum));

	if (!mMetaData)
		assert(false);
}


ScanlineRasterizer::LargeBlockPoolForTaskDispatch::~LargeBlockPoolForTaskDispatch()
{
	for(int i = 0; i < mSlotsNum; ++i)
	{
		while(mMetaData[i].mbInUse)
			std::this_thread::yield();

		if(mMetaData[i].mStorage)
			free(mMetaData[i].mStorage);
	}

	free(mMetaData);
}


void* ScanlineRasterizer::LargeBlockPoolForTaskDispatch::allocate(int index, size_t size)
{
	index %= mSlotsNum;

	while(mMetaData[index].mbInUse)
		std::this_thread::yield();

	if (mMetaData[index].mSize < size)
	{
		GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "reallocate memory block %d -> %zu\n",
			mMetaData[index].mSize, ALIGN(size, 4096));

		free(mMetaData[index].mStorage);

		mMetaData[index].mSize    = ALIGN(size, 4096);
		mMetaData[index].mStorage = malloc(mMetaData[index].mSize);
		mMetaData[index].mbInUse  = true;

		if(!mMetaData[index].mStorage)
			assert(false);
	}

	mMetaData[index].mbInUse  = true;
	return mMetaData[index].mStorage;
}


void ScanlineRasterizer::LargeBlockPoolForTaskDispatch::deallocate(int index)
{
	index %= mSlotsNum;

	assert(mMetaData[index].mbInUse);

	mMetaData[index].mbInUse = false;
}


Rasterizer::Rasterizer():
	PipeStage("Rasterizing", DrawEngine::getDrawEngine())
{
}


void Rasterizer::emit(void *data)
{
	DrawContext *dc = static_cast<DrawContext *>(data);
	onRasterizing(dc);
}


void Rasterizer::finalize()
{
}


/*
 * Calculate the barycentric coodinates of point P in this triangle
 *
 * 1. use homogeneous coordinates
 * [x0  x1  x2 ] -1    [xp ]
 * [y0  y1  y2 ]    *  [yp ]
 * [1.0 1.0 1.0]       [1.0]
 *
 * 2. use area
 */
vec3 ScanlineRasterizer::triangle::calculateBC(float xp, float yp)
{
	const vec4& v0 = mPrim.mVert[0].position();
	const vec4& v1 = mPrim.mVert[1].position();
	const vec4& v2 = mPrim.mVert[2].position();

	float a0 = ((v1.x - xp) * (v2.y - yp) - (v1.y - yp) * (v2.x - xp)) * mPrim.mAreaReciprocal;
	float a1 = ((v2.x - xp) * (v0.y - yp) - (v2.y - yp) * (v0.x - xp)) * mPrim.mAreaReciprocal;

	return vec3(a0, a1, 1.0f - a0 - a1);
	//return vec3(mMatrix * vec3(xp, yp, 1.0));
}


void ScanlineRasterizer::triangle::setActiveEdge(edge *pEdge)
{
	if(!mActiveEdge0)
	{
		mActiveEdge0 = pEdge;
	}
	else if(!mActiveEdge1)
	{
		mActiveEdge1 = pEdge;
	}
	else
	{
		GLSP_DPF(GLSP_DPF_LEVEL_FATAL, "How could this happen!\n");
		assert(0);
	}
}


void ScanlineRasterizer::triangle::unsetActiveEdge(edge *pEdge)
{
	if(pEdge == mActiveEdge0)
		mActiveEdge0 = NULL;
	else if(pEdge == mActiveEdge1)
		mActiveEdge1 = NULL;
	else
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "This edge is not active\n");
	}
}


ScanlineRasterizer::edge* ScanlineRasterizer::triangle::getAdjcentEdge(edge *pEdge)
{
	if(pEdge == mActiveEdge0)
		return mActiveEdge1;

	if(pEdge == mActiveEdge1)
		return mActiveEdge0;

	GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "getAdjcentEdge: No adjcent edge exist!\n");

	return NULL;
}


Interpolater::Interpolater():
	PipeStage("Interpolating", DrawEngine::getDrawEngine())
{
}


RasterizerWrapper::RasterizerWrapper():
	PipeStage("Rasterizer Wrapper", DrawEngine::getDrawEngine()),
	mpRasterizer(new ScanlineRasterizer),
	mpInterpolate(new PerspectiveCorrectInterpolater),
	// RasterizerWrapper doesn't own FS,
	// will be linked in when draw validate.
	mpFS(NULL),
	mpOwnershipTest(new OwnershipTester),
	mpScissorTest(new ScissorTester),
	mpStencilTest(new StencilTester),
	mpDepthTest(new ZTester),
	mpBlender(new Blender),
	mpDither(new Dither),
	mpFBWriter(new FBWriter)
{
	Interpolater *interp = dynamic_cast<Interpolater *>(mpInterpolate);
	assert(interp);

	dynamic_cast<Rasterizer *>(mpRasterizer)->SetInterpolater(interp);
}


RasterizerWrapper::~RasterizerWrapper()
{
	delete mpFBWriter;
	delete mpDither;
	delete mpBlender;
	delete mpDepthTest;
	delete mpStencilTest;
	delete mpScissorTest;
	delete mpOwnershipTest;
	delete mpInterpolate;
	delete mpRasterizer;
}


void RasterizerWrapper::initPipeline()
{
	setNextStage(mpRasterizer);
}


void RasterizerWrapper::linkPipeStages(GLContext *gc)
{
	int enables = gc->mState.mEnables;
	FragmentShader *pFS = gc->mPM.getCurrentProgram()->getFS();
	PipeStage *pFShook = static_cast<ScanlineRasterizer *>(mpRasterizer)->getFShook();

	PipeStage *last = NULL;

	if(enables & GLSP_DEPTH_TEST)
	{
		// enable early Z
		if(!pFS->getDiscardFlag() &&
			!(enables & GLSP_SCISSOR_TEST) &&
			!(enables & GLSP_STENCIL_TEST))
		{
			mpRasterizer ->setNextStage(mpDepthTest);
			mpDepthTest  ->setNextStage(mpInterpolate);
			last = mpInterpolate->setNextStage(pFShook);
		}
		else
		{
			mpRasterizer ->setNextStage(mpInterpolate);
			mpInterpolate->setNextStage(pFShook);

			if(enables & GLSP_SCISSOR_TEST)
				last = pFShook->setNextStage(mpScissorTest);

			if((enables & GL_STENCIL_TEST))
				last = last->setNextStage(mpStencilTest);

			last = last->setNextStage(mpDepthTest);
		}
	}
	else
	{
		mpRasterizer ->setNextStage(mpInterpolate);
		last = mpInterpolate->setNextStage(pFShook);
	}

	if(enables & GLSP_BLEND)
	{
		static_cast<ScanlineRasterizer *>(mpRasterizer)->pfnHSR =
			&ScanlineRasterizer::HiddenSurfaceRemovalTranslucent;
		last = last->setNextStage(mpBlender);
	}
	else
	{
		static_cast<ScanlineRasterizer *>(mpRasterizer)->pfnHSR =
			&ScanlineRasterizer::HiddenSurfaceRemoval;
	}

	if(enables & GLSP_DITHER)
		last = last->setNextStage(mpDither);

	last = last->setNextStage(mpFBWriter);
}


void RasterizerWrapper::emit(void *data)
{
	DrawContext *dc = static_cast<DrawContext *>(data);
	gRT = &dc->gc->mRT;

	getNextStage()->emit(dc);
}


void RasterizerWrapper::finalize()
{
}


void Interpolater::emit(void *data)
{
	Fsio &fsio = *static_cast<Fsio *>(data);
	const Gradience *pGrad = fsio.mpGrad;
	ScanlineRasterizer::span &sp =
		*static_cast<ScanlineRasterizer::span *>(fsio.m_priv);
	fsInput &start = sp.mStart;

	onInterpolating(start, pGrad->mGradiencesX, float(fsio.x - sp.xleft), fsio.in);

	getNextStage()->emit(data);
}


void Interpolater::finalize()
{
}


void FSHook::emit(void *data)
{
	Fsio &fsio = *static_cast<Fsio *>(data);
	ScanlineRasterizer::span &sp = *static_cast<ScanlineRasterizer::span *>(fsio.m_priv);
	DrawContext *dc = sp.mParent->mPrim.mDC;

	FragmentShader *pFS = dc->mFS;
	pFS->attachTextures(dc->mTextures);

	pFS->emit(&fsio);

	getNextStage()->emit(&fsio);
}


ScanlineRasterizer::SRHelper* ScanlineRasterizer::createGET(DrawContext *dc)
{
	GLContext *gc = dc->gc;
	DrawEngine &de = DrawEngine::getDrawEngine();
	Primlist  &pl = de.mOrderUnpreservedPrimtivesFifo;

	GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "ScanlineRasterizer::createGET start\n");

	SRHelper *hlp = new SRHelper();

	hlp->ymin = gc->mRT.height;
	hlp->ymax = 0;

	hlp->mTri.reserve(pl.size());
	auto &get = hlp->mGET;

	//TODO: clipping
	for(auto it = pl.begin(); it != pl.end(); it++)
	{
		triangle *pParent = new triangle(*it);
		hlp->mTri.push_back(pParent);

		mpInterpolate->CalculateRadiences(&pParent->mGrad);

		for(int i = 0; i < 3; i++)
		{
			vsOutput &vsout0 = it->mVert[i];
			vsOutput &vsout1 = it->mVert[(i + 1) % 3];
			const vec4 *hvert, *lvert;
			int ystart;

			int y0 = floor(vsout0.position().y + 0.5f);
			int y1 = floor(vsout1.position().y + 0.5f);

			// regarding horizontal edge, just discard it and use the other 2 edges
			if(y0 == y1)
				continue;

			if(y0 > y1)
			{
				hvert  = &(vsout0.position());
				lvert  = &(vsout1.position());
				ystart = y1;
			}
			else
			{
				hvert  = &(vsout1.position());
				lvert  = &(vsout0.position());
				ystart = y0;
			}

			// apply top-left filling convention
			edge *pEdge = new edge(pParent);

			pEdge->dx   = (hvert->x - lvert->x) / (hvert->y - lvert->y);
			pEdge->x    = lvert->x + ((ystart + 0.5f) - lvert->y) * pEdge->dx;
			pEdge->ymax = floor(hvert->y - 0.5f);

			if(hlp->ymin > ystart)
				hlp->ymin = ystart;

			if(hlp->ymax < pEdge->ymax)
				hlp->ymax = pEdge->ymax;

			auto iter = get.find(ystart);
			if(iter == get.end())
				get.insert(std::pair<int, std::vector<edge *> >(ystart, std::vector<edge *>()));
				//get[ystart] = std::vector<edge *>();

			get[ystart].push_back(pEdge);
		}
	}

	return hlp;
}

void ScanlineRasterizer::activateEdgesFromGET(SRHelper *hlp, int y)
{
	auto &get = hlp->mGET;
	auto &aet = hlp->mAET;
	auto it   = get.find(y);

	if(it != get.end())
	{
		auto &vGET = it->second;
		assert(!vGET.empty());

		for(edge *pEdge: vGET)
		{
			pEdge->mParent->setActiveEdge(pEdge);
			aet.push_back(pEdge);
		}

		hlp->mAET.sort(
			[](const edge *pEdge1, const edge *pEdge2) -> bool
			{
				return pEdge1->x < pEdge2->x;
			});
	}

	assert(aet.empty() || aet.size() % 2 == 0);

	for(edge *pEdge: aet)
		pEdge->bActive = true;

	return;
}

// Remove unvisible edges from AET.
void ScanlineRasterizer::removeEdgeFromAET(SRHelper *hlp, int y)
{
	auto &aet = hlp->mAET;
	auto it   = aet.begin();

	while(it != aet.end())
	{
		if(y > (*it)->ymax)
		{
			(*it)->mParent->unsetActiveEdge(*it);
			it = aet.erase(it);
		}
		else
		{
			it++;
		}
	}

	return;
}


// Perspective-correct interpolation
// Vp/wp = A*(V1/w1) + B*(V2/w2) + C*(V3/w3)
// wp also needs to correct:
// 1/wp = A*(1/w1) + B*(1/w2) + C*(1/w3)
// A, B, C are the barycentric coordinates
void ScanlineRasterizer::interpolate(vec3& coeff, Primitive& prim, fsInput& result)
{
	vsOutput& v0 = prim.mVert[0];
	vsOutput& v1 = prim.mVert[1];
	vsOutput& v2 = prim.mVert[2];

	float coe = 1.0f / dot(coeff, vec3(1.0, 1.0, 1.0));
	result.position().w = coe;

	for(size_t i = 1; i < v0.getRegsNum(); i++)
	{
		const vec4& reg0 = v0.getReg(i);
		const vec4& reg1 = v1.getReg(i);
		const vec4& reg2 = v2.getReg(i);

		result.getReg(i) = vec4(dot(coeff, vec3(reg0.x, reg1.x, reg2.x)) * coe,
								dot(coeff, vec3(reg0.y, reg1.y, reg2.y)) * coe,
								dot(coeff, vec3(reg0.z, reg1.z, reg2.z)) * coe,
								dot(coeff, vec3(reg0.w, reg1.w, reg2.w)) * coe);
	}
}

void ScanlineRasterizer::RenderPointFromOneSpan(span &sp, int x, int y)
{
	const triangle &tri = *(sp.mParent);
	const Primitive &prim = tri.mPrim;
	const vec4& pos0 = prim.mVert[0].position();

	size_t size = prim.mVert[0].size();
	//sp.mStart.resize(size);

	if(!sp.bInterpolated)
	{
		mpInterpolate->onInterpolating(tri.mGrad.mStarts[0],
				   tri.mGrad.mGradiencesX,
				   tri.mGrad.mGradiencesY,
				   sp.xleft + 0.5f - pos0.x,
				   y + 0.5f - pos0.y,
				   sp.mStart);

		sp.bInterpolated = true;
	}

	Fsio fsio;
	fsio.mpGrad 	= &tri.mGrad;
	fsio.x 			= x;
	fsio.y 			= y;
	fsio.mIndex 	= (gRT->height - y - 1) * gRT->width + x;
	fsio.m_priv 	= (void *)&sp;
	fsio.z 			= sp.mCurrentZ;
	fsio.in.resize(size);

	getNextStage()->emit(&fsio);
}


void ScanlineRasterizer::HiddenSurfaceRemoval(ScanlineRasterizer *pSR,
	const ActiveEdgeTableX &aetx, int x, int y)
{
	assert(!aetx.empty());

	span *sp = aetx.front();
	int  idx = (gRT->height - y - 1) * gRT->width + x;

	// early Z test
	__GET_CONTEXT();
	if(!(gc->mState.mEnables & GLSP_DEPTH_TEST) ||
		sp->mCurrentZ < gRT->pDepthBuffer[idx])
	{
		pSR->RenderPointFromOneSpan(*sp, x, y);
	}
}

void ScanlineRasterizer::HiddenSurfaceRemovalTranslucent(ScanlineRasterizer *pSR,
	const ActiveEdgeTableX &aetx, int x, int y)
{
	assert(!aetx.empty());
	int idx = (gRT->height - y - 1) * gRT->width + x;

	// early Z test
	__GET_CONTEXT();
	for(span *sp: aetx)
	{
		if(!(gc->mState.mEnables & GLSP_DEPTH_TEST) ||
			sp->mCurrentZ < gRT->pDepthBuffer[idx])
		{
			pSR->RenderPointFromOneSpan(*sp, x, y);
		}
	}
}

void ScanlineRasterizer::traversalAET(SRHelper *hlp, int y)
{
	int span_count = 0;
	ActiveEdgeTableY &aet = hlp->mAET;

	assert(aet.size() % 2 == 0);
	void *spans = mBlockPool.allocate(y, sizeof(span) * aet.size() / 2);

	// Note that AET has been sorted from left to right.
	for(auto it = aet.begin(); it != aet.end(); it++)
	{
		if((*it)->bActive != true)
			continue;

		edge *pEdge = *it;
		triangle *pParent = pEdge->mParent;
		edge *pAdjcentEdge = pParent->getAdjcentEdge(pEdge);

		assert(pAdjcentEdge->bActive);

		pEdge->bActive        = false;
		pAdjcentEdge->bActive = false;

#if 1
		// NOTE: Comment out this if there is artifact.
		if(fabs(pAdjcentEdge->x - pEdge->x) < 0.001f)
			continue;
#endif

		int xleft = ceil(pEdge->x - 0.5f);
		int xright = ceil(pAdjcentEdge->x - 0.5f);

		void *raw = reinterpret_cast<span *>(spans) + span_count;
		new(raw) span(xleft, xright, pParent);
		span_count++;
	}

	auto handler = [this, y, span_count](void *data)
	{
		ActiveEdgeTableX aetx;
		span *sp = static_cast<span *>(data);
		int curr = 0;
		int x = sp[0].xleft;

		for(int i = 0; i < span_count; ++i)
		{
			const Gradience &grad = sp[i].mParent->mGrad;
			const vec4& pos0 = sp[i].mParent->mPrim.mVert[0].position();
			sp[i].mStart.resize(grad.mGradiencesX.size());

			static_cast<PerspectiveCorrectInterpolater *>(mpInterpolate)->onInterpolating(grad.mStarts[0][0].z,
							grad.mGradiencesX[0].z,
							grad.mGradiencesY[0].z,
							sp[i].xleft + 0.5f - pos0.x,
							y + 0.5f - pos0.y,
							sp[i].mStart[0].z);

			sp[i].mCurrentZ = sp[i].mStart[0].z;
		}

		while (x < sp[span_count - 1].xright)
		{
			bool added = false, rmFront = false;

			auto it = aetx.begin();
			while(it != aetx.end())
			{
				if(x >= (*it)->xright)
				{
					if(*it == aetx.front())
						rmFront = true;

					it = aetx.erase(it);
				}
				else
				{
					it++;
				}
			}

			for(span *sp: aetx)
			{
				const Gradience &grad = sp->mParent->mGrad;
				sp->mCurrentZ += grad.mGradiencesX[0].z;
			}
			for (; curr < span_count; ++curr)
			{
				if (sp[curr].xleft == x)
				{
					if(aetx.empty() || (!rmFront && sp[curr].mCurrentZ <= aetx.front()->mCurrentZ))
						aetx.push_front(&sp[curr]);
					else
						aetx.push_back(&sp[curr]);

					added = true;
				}
				else
				{
					break;
				}
			}

			if (aetx.empty())
			{
				x = sp[curr].xleft;
				continue;
			}

			if (aetx.size() >= 2)
			{
				if (rmFront && this->pfnHSR == &HiddenSurfaceRemoval)
				{
					ActiveEdgeTableX::const_iterator nearest_iter = aetx.begin();

					for (ActiveEdgeTableX::const_iterator iter = aetx.begin(); iter != aetx.end(); ++iter)
					{
						if((*iter)->mCurrentZ < (*nearest_iter)->mCurrentZ)
						nearest_iter = iter;
					}

					if (nearest_iter != aetx.begin())
						aetx.splice(aetx.cbegin(), aetx, nearest_iter);
				}
				else if (added && this->pfnHSR == &HiddenSurfaceRemovalTranslucent)
				{
					aetx.sort(
						[](const span *pSpan1, const span *pSpan2) -> bool
						{
							return pSpan1->mCurrentZ < pSpan2->mCurrentZ;
						});
				}
			}

			this->pfnHSR(this, aetx, x, y);
			x++;
		}

		for(curr = 0; curr < span_count; ++curr)
			sp[curr].~span();

		this->mBlockPool.deallocate(y);
	};

	if (span_count > 0)
	{
		::glsp::ThreadPool &threadPool = ::glsp::ThreadPool::get();
		WorkItem *pWork = threadPool.CreateWork(handler, spans);
		threadPool.AddWork(pWork);
	}
	else
	{
		GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "no spans\n");
		mBlockPool.deallocate(y);
	}

	return;
}


void ScanlineRasterizer::advanceEdgesInAET(SRHelper *hlp)
{
	for(auto it = hlp->mAET.begin(); it != hlp->mAET.end(); it++)
	{
		(*it)->x += (*it)->dx;
	}

	return;
}

void ScanlineRasterizer::scanConversion(SRHelper *hlp)
{
	for(int i = hlp->ymin; i <= hlp->ymax; i++)
	{
		removeEdgeFromAET(hlp, i);
		activateEdgesFromGET(hlp, i);
		traversalAET(hlp, i);
		advanceEdgesInAET(hlp);
	}

	::glsp::ThreadPool::get().waitForAllTaskDone();

	GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "scanConversion done!\n");
}

void ScanlineRasterizer::finalize(SRHelper *hlp)
{
	hlp->mAET.clear();

	for(auto it = hlp->mGET.begin(); it != hlp->mGET.end(); it++)
	{
		for(auto iter = it->second.begin(); iter < it->second.end(); iter++)
		{
			delete *iter;
		}
	}

	for(auto it = hlp->mTri.begin(); it < hlp->mTri.end(); it++)
		delete *it;

	delete hlp;

	Rasterizer::finalize();
}

void ScanlineRasterizer::onRasterizing(DrawContext *dc)
{
	SRHelper *hlp = createGET(dc);

	scanConversion(hlp);

	finalize(hlp);
}

void PerspectiveCorrectInterpolater::emit(void *data)
{
	Fsio &fsio = *static_cast<Fsio *>(data);
	const Gradience *pGrad = fsio.mpGrad;
	ScanlineRasterizer::span &sp =
		*static_cast<ScanlineRasterizer::span *>(fsio.m_priv);
	fsInput &start = sp.mStart;

	onInterpolating(start, pGrad->mGradiencesX, float(fsio.x - sp.xleft), fsio.in);

	PerspectiveCorrect(fsio.in);

	getNextStage()->emit(data);
}

// Use the gradience to store partial derivatives of c/z
void PerspectiveCorrectInterpolater::CalculateRadiences(Gradience *pGrad)
{
	const Primitive &prim = pGrad->mPrim;

	size_t size = prim.mVert[0].getRegsNum();

	assert(prim.mVertNum <= 3);

	for(int i = 0; i < prim.mVertNum; ++i)
	{
		pGrad->mStarts[i].resize(size);

		const float ZReciprocal = 1.0f / prim.mVert[i].position().w;
		pGrad->mStarts[i][0] = prim.mVert[i][0];
		pGrad->mStarts[i][0].w = ZReciprocal;

		for(size_t j = 1; j < size; ++j)
		{
			pGrad->mStarts[i][j] = prim.mVert[i][j] * ZReciprocal;
		}
	}

	pGrad->mGradiencesX.resize(size);
	pGrad->mGradiencesY.resize(size);

	const vec4 &pos0 = prim.mVert[0].position();
	const vec4 &pos1 = prim.mVert[1].position();
	const vec4 &pos2 = prim.mVert[2].position();

	const float y1y2 = (pos1.y - pos2.y) * prim.mAreaReciprocal;
	const float y2y0 = (pos2.y - pos0.y) * prim.mAreaReciprocal;
	const float y0y1 = (pos0.y - pos1.y) * prim.mAreaReciprocal;

	const float x2x1 = (pos2.x - pos1.x) * prim.mAreaReciprocal;
	const float x0x2 = (pos0.x - pos2.x) * prim.mAreaReciprocal;
	const float x1x0 = (pos1.x - pos0.x) * prim.mAreaReciprocal;

#define GRADIENCE_EQUATION(i, c)	\
	pGrad->mGradiencesX[i].c = y1y2 * pGrad->mStarts[0][i].c +	\
							   y2y0 * pGrad->mStarts[1][i].c +	\
							   y0y1 * pGrad->mStarts[2][i].c;	\
	pGrad->mGradiencesY[i].c = x2x1 * pGrad->mStarts[0][i].c +	\
							   x0x2 * pGrad->mStarts[1][i].c +	\
							   x1x0 * pGrad->mStarts[2][i].c;

	pGrad->mGradiencesX[0].x = 1.0f;
	pGrad->mGradiencesX[0].y = 0.0f;

	pGrad->mGradiencesY[0].x = 0.0f;
	pGrad->mGradiencesY[0].y = 1.0f;

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

#if 1
	__GET_CONTEXT();

	int texCoordLoc = gc->mPM.getCurrentProgram()->getFS()->GetTextureCoordLocation();

	const float &dudx = pGrad->mGradiencesX[texCoordLoc].x;
	const float &dvdx = pGrad->mGradiencesX[texCoordLoc].y;
	const float &dudy = pGrad->mGradiencesY[texCoordLoc].x;
	const float &dvdy = pGrad->mGradiencesY[texCoordLoc].y;
	const float &dzdx = pGrad->mGradiencesX[0].w;
	const float &dzdy = pGrad->mGradiencesY[0].w;
	const float &z0   = pGrad->mStarts[0][0].w;
	const float &u0   = pGrad->mStarts[0][texCoordLoc].x;
	const float &v0   = pGrad->mStarts[0][texCoordLoc].y;

	pGrad->a = dudx * dzdy - dzdx * dudy;
	pGrad->b = dvdx * dzdy - dzdx * dvdy;
	pGrad->c = dudx * z0   - dzdx * u0;
	pGrad->d = dvdx * z0   - dzdx * v0;
	pGrad->e = dudy * z0   - dzdy * u0;
	pGrad->f = dvdy * z0   - dzdy * v0;
#endif
}

void PerspectiveCorrectInterpolater::onInterpolating(
									const float in,
							 		const float &gradX,
							 		const float &gradY,
							 		float stepX, float stepY,
							 		float &out)
{
	out = in + gradX * stepX + gradY * stepY;
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
}

void PerspectiveCorrectInterpolater::PerspectiveCorrect(fsInput &in)
{
	float z = 1.0f / in[0].w;

	in[0].w = z;

	// *= operator overload impl
	// out *= z;

	for(size_t i = 1; i < in.size(); ++i)
	{
		in[i] *= z;
	}
}

NS_CLOSE_GLSP_OGL()
