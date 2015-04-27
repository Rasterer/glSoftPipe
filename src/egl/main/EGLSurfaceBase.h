#pragma once

#include "EGLDisplayBase.h"

NS_OPEN_GLSP_EGL()

class EGLSurfaceBase: public EGLResourceBase
{
public:
	EGLSurfaceBase(EGLDisplayBase &dpy, EGLenum type);
	virtual ~EGLSurfaceBase() { }

	virtual bool initSurface(EGLDisplayBase *dpy, EGLNativeWindowType win) = 0;

	virtual bool swapBuffers() = 0;

	virtual bool getBuffers(void **addr, int *width, int *height) = 0;

protected:
	const EGLenum mType;

	EGLint mWidth;
	EGLint mHeight;
};

NS_CLOSE_GLSP_EGL()
