#include "EGLSurfaceBase.h"

NS_OPEN_GLSP_EGL()

EGLSurfaceBase::EGLSurfaceBase(EGLDisplayBase &dpy, EGLenum type):
	EGLResourceBase(dpy, KEGL_SURFACE_TYPE),
	mType(type)
{
}

NS_CLOSE_GLSP_EGL()
