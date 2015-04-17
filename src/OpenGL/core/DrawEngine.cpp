#include "DrawEngine.h"

#include "khronos/glcorearb.h"
#include "GLContext.h"
#include "VertexFetcher.h"
#include "Rasterizer.h"
#include "iostream" //jzb

extern int OffscreenFileGen(const size_t width, const size_t height, const std::string &map, const void *pixels);

NS_OPEN_GLSP_OGL()

using namespace std; //jzb

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
}

GLAPI void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices)
{
	__GET_CONTEXT();

	DrawEngine *de = &DrawEngine::getDrawEngine();

	de->init(); //jzb

	DrawContext *dc = new DrawContext();

	dc->gc = gc;
	gc->mDC = dc;
	dc->mMode = mode;
	dc->mFirst = 0;
	dc->mCount = count;
	dc->mDrawType = DrawContext::kElementDraw;
	dc->mIndexSize = (type == GL_UNSIGNED_INT)? 4: ((type == GL_UNSIGNED_SHORT)? 2: 1);
	dc->mIndices = indices;

	cout << "jzb: glDrawElements: 0" << endl;
	if(!de->validateState(dc))
		return;

	cout << "jzb: glDrawElements: 1" << endl;
	de->prepareToDraw(dc);
	cout << "jzb: glDrawElements: 2" << endl;
	de->emit(dc);

	de->SwapBuffers(dc);
	cout << "jzb: glDrawElements: 3" << endl;
}

DrawEngine::DrawEngine():
	mVertexFetcher(new VertexCachedFetcher()),
	mRast(new ScanlineRasterizer()),
	mFirstStage(NULL)
{
}

// TODO(done): connect all the pipeline stages
// Shaders are inserted in validateState()
void DrawEngine::init(void *dpy, IEGLBridge *bridge)
{
	mpEGLDisplay = dpy;

	assert(bridge->getBuffers);
	bridge->createGC = glsp::ogl::CreateContext;

	mpBridge = bridge;


	setFirstStage(mVertexFetcher);

	mPrimAsbl.setNextStage(&mClipper);
	mClipper.setNextStage(&mDivider);
	mDivider.setNextStage(&mMapper);
	mMapper.setNextStage(&mCuller);
	mCuller.setNextStage(mRast);
}

bool DrawEngine::validateState(DrawContext *dc)
{
	// TODO: some validation work
	GLContext *gc = dc->gc;
	VertexArrayObject *pVAO = gc->mVAOM.getActiveVAO();
	BufferObject *pElementBO = gc->mBOM.getBoundBuffer(GL_ELEMENT_ARRAY_BUFFER);

	// TODO: auto indices
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
	VertexShader *pVS = gc->mPM.getCurrentProgram()->getVS();
	FragmentShader *pFS = gc->mPM.getCurrentProgram()->getFS();

	mVertexFetcher->setNextStage(pVS);
	pVS->setNextStage(&mPrimAsbl);
	mRast->setNextStage(pFS);

	// TODO: use real view port state
	mMapper.setViewPort(0, 0, 1280, 720);

	return true;
}

void DrawEngine::beginFrame(DrawContext *dc)
{
	// TODO: use info from GLContext(fbo or egl surface)
	dc->mRT.width = 1280;
	dc->mRT.height = 720;
	dc->mRT.pColorBuffer = malloc(dc->mRT.width * dc->mRT.height * 4);
	dc->mRT.pDepthBuffer = (float *)malloc(dc->mRT.width * dc->mRT.height * sizeof(float));
	dc->mRT.pStencilBuffer = NULL;

	for(int i = 0; i < dc->mRT.width * dc->mRT.height * 4; i += 4)
	{
		*((unsigned char *)dc->mRT.pColorBuffer + i) = 0;
		*((unsigned char *)dc->mRT.pColorBuffer + i + 1) = 0;
		*((unsigned char *)dc->mRT.pColorBuffer + i + 2) = 0;
		*((unsigned char *)dc->mRT.pColorBuffer + i + 3) = 255;
	}

	for(int i = 0; i < dc->mRT.width * dc->mRT.height; i++)
	{
		*(dc->mRT.pDepthBuffer + i) = 1.0f;
	}
}

void DrawEngine::prepareToDraw(DrawContext *dc)
{
	GLContext *gc = dc->gc;

	if(!gc->mbInFrame)
	{
		beginFrame(dc);
	}
}

void DrawEngine::emit(DrawContext *dc)
{
	cout << "jzb: emit: 0" << endl;
	getFirstStage()->emit(dc);
	cout << "jzb: emit: 1" << endl;
}

void DrawEngine::SwapBuffers(DrawContext *dc)
{
	OffscreenFileGen(1280, 720, "RGBA", dc->mRT.pColorBuffer);

	free(dc->mRT.pColorBuffer);
	free(dc->mRT.pDepthBuffer);
	dc->mRT.pColorBuffer = NULL;
	dc->mRT.pDepthBuffer = NULL;
	dc->mRT.pStencilBuffer = NULL;
}

DrawEngine DrawEngine::DE;

bool iglCreateScreen(void *dpy, IEGLBridge *bridge)
{
	DrawEngine &de = DrawEngine::getDrawEngine();

	de.init(dpy, bridge);

	return true;
}

NS_CLOSE_GLSP_OGL()
