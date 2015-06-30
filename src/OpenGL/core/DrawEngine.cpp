#include "DrawEngine.h"

#include "khronos/GL/glcorearb.h"
#include "common/IEGLBridge.h"
#include "GLContext.h"
#include "VertexFetcher.h"
#include "Rasterizer.h"
#include "PrimitiveAssembler.h"
#include "Clipper.h"
#include "FaceCuller.h"
#include "PerspectiveDivider.h"
#include "ScreenMapper.h"
#include "ThreadPool.h"

using glsp::ogl::DrawEngine;
using glsp::ogl::DrawContext;
using glsp::IEGLBridge;

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

	if(!de->validateState(dc))
		return;

	de->prepareToDraw(dc);
	de->emit(dc);

	delete dc;
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

	de->prepareToDraw(dc);
	de->emit(dc);

	delete dc;
	gc->mDC = NULL;
}

NS_OPEN_GLSP_OGL()

namespace {

bool swapBuffers()
{
	return DrawEngine::getDrawEngine().SwapBuffers();
}

} //namespace

DrawEngine::DrawEngine():
	mFirstStage(NULL),
	mpEGLDisplay(NULL),
	mpBridge(NULL)
{
}

DrawEngine::~DrawEngine()
{
	deinitTLS();

	delete mVertexFetcher;
	delete mPrimAsbl;
	delete mClipper;
	delete mDivider;
	delete mMapper;
	delete mCuller;
	delete mRast;
}

// TODO(done): connect all the pipeline stages
// Shaders are inserted in validateState()
void DrawEngine::init(void *dpy, IEGLBridge *bridge)
{
	mpEGLDisplay = dpy;

	assert(bridge->getBuffers);

	bridge->createGC    = CreateContext;
	bridge->destroyGC   = DestroyContext;
	bridge->makeCurrent = MakeCurrent;
	bridge->swapBuffers = swapBuffers;

	initTLS();

	mpBridge = bridge;

	initPipeline();

	ThreadPool &threadPool = ThreadPool::get();

	threadPool.Initialize();
}

void DrawEngine::initPipeline()
{
	mVertexFetcher = new VertexCachedFetcher();
	mPrimAsbl      = new PrimitiveAssembler();
	mClipper       = new Clipper();
	mDivider       = new PerspectiveDivider();
	mMapper        = new ScreenMapper();
	mCuller        = new FaceCuller();
	mRast          = new RasterizerWrapper();

	assert(mVertexFetcher &&
		   mPrimAsbl      &&
		   mClipper       &&
		   mDivider       &&
		   mMapper        &&
		   mCuller        &&
		   mRast
		   );

	setFirstStage(mVertexFetcher);

	mPrimAsbl->setNextStage(mClipper);
	mClipper ->setNextStage(mDivider);
	mDivider ->setNextStage(mMapper);

	mRast->initPipeline();
}

void DrawEngine::linkPipeStages(GLContext *gc)
{
	int enables = gc->mState.mEnables;
	VertexShader *pVS = gc->mPM.getCurrentProgram()->getVS();

	mVertexFetcher->setNextStage(pVS);
	pVS->setNextStage(mPrimAsbl);

	if(enables & GLSP_CULL_FACE)
	{
		mMapper->setNextStage(mCuller);
		mCuller->setNextStage(mRast);
	}
	else
	{
		mMapper->setNextStage(mRast);
	}

	mRast->linkPipeStages(gc);
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

	if(!gc->mTM.validateTextureState(pVS, pFS))
		ret = false;

	return ret;
}

void DrawEngine::beginFrame(GLContext *gc)
{
	RenderTarget &rt = gc->mRT;
	GLViewport   &vp = gc->mState.mViewport;

	// TODO: use info from GLContext(fbo or egl surface)
	if(!mpBridge->getBuffers(gc->mpEGLContext,
						     &rt.pColorBuffer,
						     &rt.width,
						     &rt.height))
	{
		return;
	}

	// window resize, reset the viewport
	if((rt.width  != vp.width) ||
	   (rt.height != vp.height))
	{
		gc->applyViewport(0, 0, rt.width, rt.height);
	}

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

	for(int i = 0; i < rt.width * rt.height; i++)
	{
		*(rt.pDepthBuffer + i) = 1.0f;
	}

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

#if 0
	// TODO(done): use real view port state
	if(!(gc->mEmitFlag & EMIT_FLAG_VIEWPORT))
	{
		mMapper->setViewPort(0, 0,
							 gc->mRT.width,
							 gc->mRT.height);
	}
	else
	{
		// FIXME: how to draw if viewport exceed the buffer boundary
		int w = gc->mState.mViewport.width;
		int h = gc->mState.mViewport.height;

		if((gc->mState.mViewport.x + gc->mState.mViewport.width) >
			gc->mRT.width)
			w = gc->mRT.width - gc->mState.mViewport.x;

		if((gc->mState.mViewport.y + gc->mState.mViewport.height) >
			gc->mRT.height)
			h = gc->mRT.height - gc->mState.mViewport.y;


		mMapper->setViewPort(gc->mState.mViewport.x,
							 gc->mState.mViewport.y,
							 w, h);
	}
#endif
}

void DrawEngine::emit(DrawContext *dc)
{
	getFirstStage()->emit(dc);
}

bool DrawEngine::SwapBuffers()
{
	__GET_CONTEXT();

	gc->mbInFrame = false;

	free(gc->mRT.pDepthBuffer);
	gc->mRT.pColorBuffer = NULL;
	gc->mRT.pDepthBuffer = NULL;
	gc->mRT.pStencilBuffer = NULL;

	return true;
}


NS_CLOSE_GLSP_OGL()


namespace glsp {

bool iglCreateScreen(void *dpy, IEGLBridge *bridge)
{
	DrawEngine &de = DrawEngine::getDrawEngine();

	de.init(dpy, bridge);

	return true;
}

} //namespace glsp