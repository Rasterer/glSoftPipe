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
#include "compiler.h"
#include "khronos/GL/glspcorearb.h"


namespace glsp {

GLAPI void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	GLContext *gc = g_GC;

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

	de->prepareToDraw();
	de->emit(dc);

	gc->mDC = NULL;
}

GLAPI void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	GLContext *gc = g_GC;

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

	de->prepareToDraw();
	de->emit(dc);

	gc->mDC = NULL;
}

GLAPI void APIENTRY glClear (GLbitfield mask)
{
	DrawEngine &de = DrawEngine::getDrawEngine();

	if (!de.validateState(nullptr))
		return;

	de.prepareToDraw();
	de.PerformClear(mask);
}

GLAPI void APIENTRY glClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	GLContext *gc = g_GC;

	gc->mState.mClearState.red   = red;
	gc->mState.mClearState.green = green;
	gc->mState.mClearState.blue = blue;
	gc->mState.mClearState.alpha = alpha;
}

GLAPI void APIENTRY glClearStencil (GLint s)
{
	GLContext *gc = g_GC;

	gc->mState.mClearState.stencil = s;
}

GLAPI void APIENTRY glClearDepth (GLdouble depth)
{
	GLContext *gc = g_GC;

	gc->mState.mClearState.depth = depth;
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
	mGLContext(nullptr),
	mDrawCount(0)
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
	delete mTBDR;
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

	delete mGLContext;
}

// TODO(done): connect all the pipeline stages
// Shaders are inserted in validateState()
void DrawEngine::init()
{
	mGLContext = new GLContext(4, 0, *this);
	MakeCurrent(mGLContext);

	initPipeline();
}

void DrawEngine::SetNativeWindowInfo(NWMWindowInfo &win_info)
{
	mGLContext->mFBOM.GetDefaultFBO()->DefaultFBOInitRenderTarget(win_info.width, win_info.height, win_info.format);
	mGLContext->applyViewport(0, 0, win_info.width, win_info.height);
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
	mTBDR          = new TBDR(*this);

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

	mRast->setFirstChild(mTBDR);
}

void DrawEngine::linkGeomertryPipeStages()
{
	int enables = mGLContext->mState.mEnables;
	VertexShader *pVS = mGLContext->mPM.getCurrentProgram()->getVS();

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
}

void DrawEngine::linkRasterizerPipeStages()
{
	// NOTE:
	// Rasterizer stages has complex logic and are tightly coupled.
	// So don't link them together here, instead, let TBDR handle the details. 
#if 0
	int enables = mGLContext->mState.mEnables;
	FragmentShader *pFS = mGLContext->mPM.getCurrentProgram()->getFS();

	PipeStage *last = NULL;

	if(enables & GLSP_DEPTH_TEST)
	{
		// enable early Z
		if(!pFS->getDiscardFlag() &&
			!(enables & GLSP_SCISSOR_TEST) &&
			!(enables & GLSP_STENCIL_TEST))
		{
			if (mFlushTriggerBySwapBuffer)
			{
				mTBDR->setNextStage(mInterpolater);
				last = mInterpolater->setNextStage(pFS);
			}
			else
			{
				mTBDR->setNextStage(mDepthTest);
				mDepthTest->setNextStage(mInterpolater);
				last = mInterpolater->setNextStage(pFS);
			}
		}
		else
		{
			mTBDR->setNextStage(mInterpolater);
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
		mTBDR ->setNextStage(mInterpolater);
		last = mInterpolater->setNextStage(pFS);
	}

	if(enables & GLSP_BLEND)
	{
		last = last->setNextStage(mBlender);
	}

	if(enables & GLSP_DITHER)
		last = last->setNextStage(mDither);

	last = last->setNextStage(mFBWriter);
#endif
}

// OPT: use dirty flag to optimize
bool DrawEngine::validateState(DrawContext *dc)
{
	// TODO: some validation work
	bool ret = true;

	if (dc)
	{
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
		{
			ret = false;
			goto out;
		}

		dc->mFS = pFS;
	}
	else
	{
		// glClear path
	}

	ret = mGLContext->mFBOM.ValidateFramebufferStatus(mGLContext);
	if (ret && dc)
		dc->mDrawID = mDrawCount++;
out:
	return ret;
}

void DrawEngine::beginFrame(GLContext *gc)
{
	gc->mbInFrame = true;
}

void DrawEngine::prepareToDraw()
{
	if(!mGLContext->mbInFrame)
	{
		beginFrame(mGLContext);
	}

	mGLContext->mFBOM.GetDrawFBO()->SetHasPendingDrawCommand();
}

void DrawEngine::PerformClear(unsigned int mask)
{
	RenderTarget &rt = mGLContext->mRT;

	if (mask & GL_COLOR_BUFFER_BIT && rt.pColorBuffer)
	{
		uint8_t r = static_cast<uint8_t>(mGLContext->mState.mClearState.red   * 256.0f);
		uint8_t g = static_cast<uint8_t>(mGLContext->mState.mClearState.green * 256.0f);
		uint8_t b = static_cast<uint8_t>(mGLContext->mState.mClearState.blue  * 256.0f);
		uint8_t a = static_cast<uint8_t>(mGLContext->mState.mClearState.alpha * 256.0f);

		uint32_t color = (r << 0) | (g << 8) | (b << 16) | (a << 24);
		__m128i vColor = _mm_set1_epi32(color);

		int count = rt.width * rt.height;
		__m128i *addr = (__m128i *)rt.pColorBuffer;

		// Assume 64 bytes cache line size
		for(int i = 0; i < count; i += 16, addr += 4)
		{
			_mm_stream_si128(addr    , vColor);
			_mm_stream_si128(addr + 1, vColor);
			_mm_stream_si128(addr + 2, vColor);
			_mm_stream_si128(addr + 3, vColor);
		}
	}

	if (mask & GL_DEPTH_BUFFER_BIT && rt.pDepthBuffer)
	{
		// Defer clear until the rasterizer stage begin,
		// and that will only clear the per-tile depth buffer
		// rather than the big one from render target, improving cache locality.
		mTBDR->SetDepthClearFlag();
	}

	if (mask & GL_STENCIL_BUFFER_BIT && rt.pStencilBuffer)
	{
		// TODO: clear stencil buffer
	}
}

void DrawEngine::emit(DrawContext *dc)
{
	dc->m_pNext = mDrawContextList;
	mDrawContextList = dc;
	linkGeomertryPipeStages();
	getFirstStage()->emit(dc);
}

bool DrawEngine::SwapBuffers(NWMBufferToDisplay *buf)
{
	Flush(true);

	buf->addr   = mGLContext->mRT.pColorBuffer;
	buf->width  = mGLContext->mRT.width;
	buf->height = mGLContext->mRT.height;
	buf->format = mGLContext->mRT.format;

	mGLContext->mbInFrame = false;

	return true;
}

void DrawEngine::Flush(bool swap_buffer)
{
	linkRasterizerPipeStages();

	bool depth_only = mGLContext->mFBOM.GetDrawFBO()->IsDepthOnly();
	mTBDR->FlushDisplayLists(swap_buffer, depth_only);

	DrawContext *dc = mDrawContextList;
	while(dc)
	{
		DrawContext *tmp = dc;
		dc = dc->m_pNext;

		for (int i = 0; i < MAX_TEXTURE_UNITS; ++i)
		{
			if (tmp->mTextures[i])
				tmp->mTextures[i]->DecRef();
		}
		delete tmp;
	}
	mDrawContextList = NULL;
	mDrawCount = 0;
	mGLContext->mFBOM.GetDrawFBO()->ClearHasPendingDrawCommand();
}

bool glspCreateRender()
{
	DrawEngine &de = DrawEngine::getDrawEngine();

	de.init();

	return true;
}

bool glspSwapBuffers(NWMBufferToDisplay *buf)
{
	return DrawEngine::getDrawEngine().SwapBuffers(buf);
}

void glspSetNativeWindowInfo(NWMWindowInfo *win_info)
{
	if (win_info)
		DrawEngine::getDrawEngine().SetNativeWindowInfo(*win_info);
}

} // namespace glsp
