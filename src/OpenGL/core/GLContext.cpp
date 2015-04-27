#include <pthread.h>

#include "khronos/GL/glcorearb.h"
#include "GLContext.h"

GLAPI void APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GET_CONTEXT();

	gc->mState.mViewport.x      = x;
	gc->mState.mViewport.y      = y;
	gc->mState.mViewport.width  = width;
	gc->mState.mViewport.height = height;

	gc->mEmitFlag |= EMIT_FLAG_VIEWPORT;
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

NS_CLOSE_GLSP_OGL()
