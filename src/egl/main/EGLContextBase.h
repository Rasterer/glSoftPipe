#pragma once

#include "khronos/EGL/egl.h"
#include "common/glsp_defs.h"
#include "EGLDisplayBase.h"

NS_OPEN_GLSP_EGL()

class EGLSurfaceBase;

class EGLContextBase: public EGLResourceBase
{
public:
	EGLContextBase(EGLDisplayBase &dpy);
	virtual ~EGLContextBase();

	bool initContext();

	void bindSurface(EGLSurfaceBase *read, EGLSurfaceBase *draw)
	{
		mSurfaceRead = read;
		mSurfaceDraw = draw;
	}

	EGLSurfaceBase* getDrawSurface() const { return mSurfaceDraw; }
	EGLSurfaceBase* getReadSurface() const { return mSurfaceRead; }

	void* getClientGC() const { return mClientGC; }

private:
	void *mClientGC;

	EGLint     mClientAPI;
	EGLint     mClientVersionMajor;
	EGLint     mClientVersionMinor;

	EGLSurfaceBase *mSurfaceDraw;
	EGLSurfaceBase *mSurfaceRead;
};

NS_CLOSE_GLSP_EGL()
