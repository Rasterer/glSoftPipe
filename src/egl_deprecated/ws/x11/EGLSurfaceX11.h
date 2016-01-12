#pragma once

#define _XSERVER64

#include <unordered_map>

#include <xf86drm.h>
#include <xcb/xcb.h>
#include "EGLSurfaceBase.h"

extern "C" {
	#include <intel_bufmgr.h>
	#include <i915_drm.h>
	#include <drm_fourcc.h>
}


NS_OPEN_GLSP_EGL()

class EGLSurfaceX11: public EGLSurfaceBase
{
public:
	EGLSurfaceX11(EGLDisplayBase &dpy, EGLenum type);
	virtual ~EGLSurfaceX11();

	virtual bool initSurface(EGLDisplayBase *dpy, EGLNativeWindowType win);

	virtual bool swapBuffers();

	virtual bool getBuffers(void **addr, int *width, int *height);

private:
	::xcb_drawable_t mXDrawable;
	::xcb_gcontext_t mXContext;

	::drm_intel_bo  *mCurrentBO;
	std::unordered_map<uint32_t, ::drm_intel_bo *> mBufferCache;
};

NS_CLOSE_GLSP_EGL()
