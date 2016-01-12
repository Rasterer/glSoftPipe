#include "DrawEngine.h"

#include <cstdlib>

#include "GLContext.h"
#include "VertexFetcher.h"
#include "Rasterizer.h"
#include "PrimitiveAssembler.h"
#include "Clipper.h"
#include "FaceCuller.h"
#include "PerspectiveDivider.h"
#include "ScreenMapper.h"
#include "TBDR.h"
#include "PixelBackend.h"
#include "ThreadPool.h"


namespace glsp {
#include "khronos/GL/glcorearb.h"

GLAPI void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	__GET_CONTEXT();

	DrawEngine *de = &DrawEngine::getDrawEngine();

	DrawContext *dc = new DrawContext();

	// encapsulate the DrawContext prepare work
	dc->gc = gc;
	gc->mDC = dc;
	dc->mMode = mode;
	dc->mFirst = first;
	dc->mCount = count;
	dc->mDrawType = DrawContext::kArrayDraw;
	dc->mIndices = 0;

	if(!de->validateState(dc))
		return;

	dc->m_pNext = de->getDrawContextList();
	de->setDrawContextList(dc);
	de->prepareToDraw(dc);
	de->emit(dc);

	gc->mDC = NULL;
}

GLAPI void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	__GET_CONTEXT();

	DrawEngine *de = &DrawEngine::getDrawEngine();

	DrawContext *dc = new DrawContext();

	dc->gc = gc;
	gc->mDC = dc;
	dc->mMode = mode;
	dc->mFirst = 0;
	dc->mCount = count;
	dc->mDrawType = DrawContext::kElementDraw;
	dc->mIndexSize = (type == GL_UNSIGNED_INT)? 4: ((type == GL_UNSIGNED_SHORT)? 2: 1);
	dc->mIndices = indices;

	if(!de->validateState(dc))
		return;

	dc->m_pNext = de->getDrawContextList();
	de->setDrawContextList(dc);
	de->prepareToDraw(dc);
	de->emit(dc);

	gc->mDC = NULL;
}


class GeometryStage: public PipeStageChain
{
public:
	GeometryStage():
		PipeStageChain("GeometryStage", DrawEngine::getDrawEngine())
	{
	}

	~GeometryStage() = default;

	virtual void emit(void *data)
	{
		DrawContext *dc = static_cast<DrawContext *>(data);
		mFirstChild->emit(dc);

		// Wait for all the geometry tasks done.
		::glsp::ThreadPool::get().waitForAllTaskDone();

		// NOTE: used to switch b/w immediate render and deferred render.
		// Defer and accumulate the primitive lists.
		//getNextStage()->emit(dc);
	}

private:
};

class RasterizationStage: public PipeStageChain
{
public:
	RasterizationStage():
		PipeStageChain("RasterizationStage", DrawEngine::getDrawEngine())
	{
	}

	~RasterizationStage() = default;

	virtual void emit(void *data)
	{
		DrawContext *dc = static_cast<DrawContext *>(data);
		mFirstChild->emit(dc);
	}

private:
};

DrawEngine::DrawEngine():
	mDrawContextList(nullptr),
	mFirstStage(nullptr),
	mGLContext(nullptr)
{
}

DrawEngine::~DrawEngine()
{
	delete mVertexFetcher;
	delete mPrimAsbl;
	delete mClipper;
	delete mDivider;
	delete mMapper;
	delete mCuller;
	delete mBinning;
	delete mRasterizer;
	delete mInterpolater;
	delete mOwnershipTest;
	delete mScissorTest;
	delete mStencilTest;
	delete mDepthTest;
	delete mBlender;
	delete mDither;
	delete mFBWriter;

	delete mRast;
	delete mGeometry;

	DestroyContext(mGLContext);
}

// TODO(done): connect all the pipeline stages
// Shaders are inserted in validateState()
void DrawEngine::init(NWMCallBacks *call_backs)
{
	assert(call_backs);
	mNativeWindowCallBacks = *call_backs;

	mGLContext = CreateContext(4, 0);
	MakeCurrent(mGLContext);

	NWMWindowInfo win_info;
	mNativeWindowCallBacks.GetWindowInfo(&win_info);

	mGLContext->mRT.width  = win_info.width;
	mGLContext->mRT.height = win_info.height;
	mGLContext->mRT.format = win_info.format;
	mGLContext->applyViewport(0, 0, win_info.width, win_info.height);

	initPipeline();
}

void DrawEngine::initPipeline()
{
	mVertexFetcher = new VertexCachedFetcher();
	mPrimAsbl      = new PrimitiveAssembler();
	mClipper       = new Clipper();
	mDivider       = new PerspectiveDivider();
	mMapper        = new ScreenMapper();
	mCuller        = new FaceCuller();
	mBinning       = new Binning();
	mRasterizer    = new TBDR();

	mInterpolater  = new PerspectiveCorrectInterpolater();
	mOwnershipTest = new OwnershipTester();
	mScissorTest   = new ScissorTester();
	mStencilTest   = new StencilTester();
	mDepthTest     = new ZTester();
	mBlender       = new Blender();
	mDither        = new Dither();
	mFBWriter      = new FBWriter();

	mGeometry      = new GeometryStage();
	mRast          = new RasterizationStage();

	setFirstStage(mGeometry);

	mPrimAsbl->setNextStage(mClipper);
	mClipper ->setNextStage(mDivider);
	mDivider ->setNextStage(mMapper);

	mGeometry->setFirstChild(mVertexFetcher);
	mGeometry->setLastChild(mBinning);
	mGeometry->setNextStage(mRast);

	mRast->setFirstChild(mRasterizer);
}

void DrawEngine::linkPipeStages(GLContext *gc)
{
	int enables = gc->mState.mEnables;
	VertexShader *pVS = gc->mPM.getCurrentProgram()->getVS();
	FragmentShader *pFS = gc->mPM.getCurrentProgram()->getFS();

	mVertexFetcher->setNextStage(pVS);
	pVS->setNextStage(mPrimAsbl);

	if(enables & GLSP_CULL_FACE)
	{
		mMapper->setNextStage(mCuller);
		mCuller->setNextStage(mBinning);
	}
	else
	{
		mMapper->setNextStage(mBinning);
	}

	PipeStage *last = NULL;

	if(enables & GLSP_DEPTH_TEST)
	{
		// enable early Z
		if(!pFS->getDiscardFlag() &&
			!(enables & GLSP_SCISSOR_TEST) &&
			!(enables & GLSP_STENCIL_TEST))
		{
			// TODO: need formal zero depth load/store OPT.
			//mRasterizer ->setNextStage(mDepthTest);
			//mDepthTest  ->setNextStage(mInterpolater);
			mRasterizer ->setNextStage(mInterpolater);
			last = mInterpolater->setNextStage(pFS);
		}
		else
		{
			mRasterizer  ->setNextStage(mInterpolater);
			mInterpolater->setNextStage(pFS);

			if(enables & GLSP_SCISSOR_TEST)
				last = pFS->setNextStage(mScissorTest);

			if((enables & GL_STENCIL_TEST))
				last = last->setNextStage(mStencilTest);

			last = last->setNextStage(mDepthTest);
		}
	}
	else
	{
		mRasterizer ->setNextStage(mInterpolater);
		last = mInterpolater->setNextStage(pFS);
	}

	if(enables & GLSP_BLEND)
	{
		last = last->setNextStage(mBlender);
	}

	if(enables & GLSP_DITHER)
		last = last->setNextStage(mDither);

	last = last->setNextStage(mFBWriter);
}

// OPT: use dirty flag to optimize
bool DrawEngine::validateState(DrawContext *dc)
{
	// TODO: some validation work
	bool ret = true;
	GLContext *gc = dc->gc;
	VertexArrayObject *pVAO = gc->mVAOM.getActiveVAO();
	BufferObject *pElementBO = gc->mBOM.getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);

	for(size_t i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
	{
		if(pVAO->mAttribEnables & (1 << i))
		{
			VertexAttribState *pVAS = &pVAO->mAttribState[i];
			
			if(pVAS->mBO == NULL && pVAS->mOffset == 0)
				return false;
		}
	}

	if(dc->mDrawType == DrawContext::kElementDraw)
	{
		if(!pElementBO && !dc->mIndices)
			return false;
	}

	// link the shaders to the pipeline
	VertexShader   *pVS = gc->mPM.getCurrentProgram()->getVS();
	FragmentShader *pFS = gc->mPM.getCurrentProgram()->getFS();

	if(!gc->mTM.validateTextureState(pVS, pFS, dc))
		ret = false;

	dc->mFS = pFS;

	return ret;
}

void DrawEngine::beginFrame(GLContext *gc)
{
	RenderTarget &rt = gc->mRT;

	NWMWindowInfo win_info;
	mNativeWindowCallBacks.GetWindowInfo(&win_info);

	// TODO: check buffer format compatibility
	bool size_change = ((win_info.width != rt.width) || (win_info.height != rt.height));

	if (size_change)
	{
		if (rt.pColorBuffer)
		{
			free(rt.pColorBuffer);
			rt.pColorBuffer = nullptr;
		}

		if (rt.pDepthBuffer)
		{
			free(rt.pDepthBuffer);
			rt.pDepthBuffer = nullptr;
		}

		rt.width  = win_info.width;
		rt.height = win_info.height;
		rt.format = win_info.format;
		gc->applyViewport(0, 0, rt.width, rt.height);
	}

	if (!rt.pColorBuffer)
		rt.pColorBuffer = malloc(rt.width * rt.height * 4);

	if (!rt.pDepthBuffer)
		rt.pDepthBuffer = (float *)malloc(rt.width * rt.height * sizeof(float));

	// TODO: impl stencil buffer
	rt.pStencilBuffer = NULL;

	// TODO: move to glClear()
	for(int i = 0; i < rt.width * rt.height * 4; i += 4)
	{
		*((unsigned char *)rt.pColorBuffer + i) = 0;
		*((unsigned char *)rt.pColorBuffer + i + 1) = 0;
		*((unsigned char *)rt.pColorBuffer + i + 2) = 0;
		*((unsigned char *)rt.pColorBuffer + i + 3) = 255;
	}

	//for(int i = 0; i < rt.width * rt.height; i++)
	//{
		//*(rt.pDepthBuffer + i) = 1.0f;
	//}

	gc->mbInFrame = true;
}

void DrawEngine::prepareToDraw(DrawContext *dc)
{
	GLContext *gc = dc->gc;

	if(!gc->mbInFrame)
	{
		beginFrame(gc);
	}

	linkPipeStages(gc);
}

void DrawEngine::emit(DrawContext *dc)
{
	getFirstStage()->emit(dc);
}

bool DrawEngine::SwapBuffers(NWMBufferToDisplay *buf)
{
	DrawContext *dc = mDrawContextList;

	getRastStage()->emit(dc);

	while(dc)
	{
		DrawContext *tmp = dc;
		dc = dc->m_pNext;
		delete tmp;
	}
	mDrawContextList = NULL;

	buf->addr   = mGLContext->mRT.pColorBuffer;
	buf->width  = mGLContext->mRT.width;
	buf->height = mGLContext->mRT.height;
	buf->format = mGLContext->mRT.format;

	mGLContext->mbInFrame = false;

	return true;
}


bool glspCreateRender(NWMCallBacks *call_backs)
{
	DrawEngine &de = DrawEngine::getDrawEngine();

	de.init(call_backs);

	return true;
}

bool glspSwapBuffers(NWMBufferToDisplay *buf)
{
	DrawEngine &de = DrawEngine::getDrawEngine();

	return de.SwapBuffers(buf);
}


} // namespace glsp
