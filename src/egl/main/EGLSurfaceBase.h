#pragma once

#include "EGLDisplayBase.h"

NS_OPEN_GLSP_EGL()

class EGLSurfaceBase: public EGLResourceBase
{
public:
	EGLSurfaceBase(EGLDisplayBase &dpy, EGLenum type);
	virtual ~EGLSurfaceBase() { }

	virtual bool initSurface() = 0;

	virtual bool swapBuffers() = 0;

private:
	const EGLenum mType;

	EGLint mWidth;
	EGLint mHeight;
};

NS_CLOSE_GLSP_EGL()
