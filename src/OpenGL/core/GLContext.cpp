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


NS_OPEN_GLSP_OGL()

static pthread_key_t TLSKey;

void initTLS()
{
	pthread_key_create(&TLSKey, NULL);
}

void deinitTLS()
{
	pthread_key_delete(TLSKey);
}

static void setCurrentContext(void *gc)
{
	pthread_setspecific(TLSKey, gc);
}

GLContext* getCurrentContext()
{
	return (GLContext *)pthread_getspecific(TLSKey);
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
		__SET_CONTEXT(NULL);

	delete _del;
}

void MakeCurrent(void *gc)
{
	__SET_CONTEXT(gc);
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