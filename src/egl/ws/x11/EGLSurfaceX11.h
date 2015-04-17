#pragma once

#include <xcb/xcb.h>
#include "EGLSurfaceBase.h"

NS_OPEN_GLSP_EGL()

class EGLSurfaceX11: public EGLSurfaceBase
{
public:
	EGLSurfaceX11(EGLDisplayBase &dpy, EGLenum type);
	virtual ~EGLSurfaceX11();

	virtual bool initSurface();

	virtual bool swapBuffers();

private:
	xcb_window_t   mXWindow;
	xcb_gcontext_t mXContext;
};

NS_CLOSE_GLSP_EGL()
