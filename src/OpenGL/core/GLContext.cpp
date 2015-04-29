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

	vp.x      = x;
	vp.y      = y;
	vp.width  = width;
	vp.height = height;

	gc->applyViewport();

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

void setCurrentContext(void *gc)
{
	pthread_setspecific(TLSKey, gc);
}

GLContext * getCurrentContext()
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

bool MakeCurrent(GLContext *gc)
{
	__SET_CONTEXT(gc);
	return true;
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
	mState.mViewport.x      = 0;
	mState.mViewport.y      = 0;
	mState.mViewport.width  = 0;
	mState.mViewport.height = 0;
}

void GLContext::applyViewport()
{
	GLViewport &vp = mState.mViewport;

	vp.xScale = vp.width  / 2;
	vp.yScale = vp.height / 2;

	vp.xCenter = vp.x + vp.xScale;
	vp.yCenter = vp.y + vp.yScale;
}
NS_CLOSE_GLSP_OGL()
