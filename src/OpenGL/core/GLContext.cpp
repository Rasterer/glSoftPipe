#include <pthread.h>

#include "khronos/GL/glcorearb.h"
#include "GLContext.h"


using glsp::ogl::GLContext;
using glsp::ogl::GLViewport;

GLAPI void APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GET_CONTEXT();
	GLViewport &vp = gc->mState.mViewport;

	if(vp.x == y && vp.y == y &&
		vp.width == width && vp.height == height)
		return;
	else
		gc->applyViewport(x, y, width, height);

	return;
}

GLAPI void APIENTRY glEnable (GLenum cap)
{
	__GET_CONTEXT();

	if(cap == GL_DEPTH_TEST)
		gc->mState.mEnables |= GLSP_DEPTH_TEST;
}

NS_OPEN_GLSP_OGL()

static GLContext *sGC = nullptr;

static void setCurrentContext(void *gc)
{
	sGC = static_cast<GLContext *>(gc);
}

GLContext* getCurrentContext()
{
	return sGC;
}

void* CreateContext(void *EglCtx, int major, int minor)
{
	GLContext *gc = new GLContext(EglCtx, major, minor);
	return gc;
}

void DestroyContext(void *_gc)
{
	__GET_CONTEXT();
	GLContext *_del = static_cast<GLContext *>(_gc);

	if(gc == _del)
		__SET_CONTEXT(nullptr);

	delete _del;
}

void MakeCurrent(void *gc)
{
	setCurrentContext(gc);
}

GLContext::GLContext(void *EglCtx, int major, int minor):
	mEmitFlag(0),
	mbInFrame(false),
	mpEGLContext(EglCtx),
	mVersionMajor(major),
	mVersionMinor(minor)
{
	initGC();
}

GLContext::~GLContext()
{
	if (mRT.pDepthBuffer)
	{
		free(mRT.pDepthBuffer);
	}

	if (mRT.pStencilBuffer)
	{
		free(mRT.pStencilBuffer);
	}
}

void GLContext::initGC()
{
	applyViewport(0, 0, 0, 0);

	mState.mEnables = 0;
}

void GLContext::applyViewport(int x, int y, int width, int height)
{
	GLViewport &vp = mState.mViewport;

	vp.x      = x;
	vp.y      = y;
	vp.width  = width;
	vp.height = height;

	vp.xScale = vp.width  / 2;
	vp.yScale = vp.height / 2;

	vp.xCenter = vp.x + vp.xScale;
	vp.yCenter = vp.y + vp.yScale;
}


NS_CLOSE_GLSP_OGL()