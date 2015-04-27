#include "EGLContextBase.h"

NS_OPEN_GLSP_EGL()

EGLContextBase::EGLContextBase(EGLDisplayBase &dpy):
	EGLResourceBase(dpy, KEGL_CONTEXT_TYPE),
	mClientGC(NULL),
	mSurfaceDraw(NULL),
	mSurfaceRead(NULL)
{
}

bool EGLContextBase::initContext()
{
	mClientAPI = EGL_OPENGL_API;
	mClientVersionMajor = 4;
	mClientVersionMinor = 5;

	switch(mClientAPI)
	{
		// only support OpenGL yet.
		case EGL_OPENGL_API:
			mClientGC = getDisplay().getEGLBridge()->createGC(this, mClientVersionMajor, mClientVersionMinor);
			break;

		default:
			break;
	}

	return (mClientGC != NULL);
}

EGLContextBase::~EGLContextBase()
{
	if(mClientGC)
	{
		switch(mClientAPI)
		{
			case EGL_OPENGL_API:
				getDisplay().getEGLBridge()->destroyGC(mClientGC);
				break;

			default:
				break;
		}
	}
}

NS_CLOSE_GLSP_EGL()
