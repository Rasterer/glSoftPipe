#pragma once

#include "khronos/EGL/egl.h"
#include "EGLDisplayBase.h"

NS_OPEN_GLSP_EGL()

class EGLSurfaceBase;

class EGLContextBase: public EGLResourceBase
{
public:
	EGLContextBase(EGLDisplayBase &dpy);
	virtual ~EGLContextBase();

	bool initContext();

	void bindSurface(EGLSurfaceBase *read, EGLSurfaceBase *write)
	{
		mSurfaceWrite = write;
		mSurfaceRead  = read;
	}

private:
	void *mClientGC;

	EGLint     mClientAPI;
	EGLint     mClientVersionMajor;
	EGLint     mClientVersionMinor;

	EGLSurfaceBase *mSurfaceWrite;
	EGLSurfaceBase *mSurfaceRead;
};

NS_CLOSE_GLSP_EGL()
