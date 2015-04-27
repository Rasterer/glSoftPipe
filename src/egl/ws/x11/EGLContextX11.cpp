#include "EGLContextX11.h"

#include "EGLDisplayBase.h"

NS_OPEN_GLSP_EGL()

EGLContextX11::EGLContextX11(EGLDisplayBase &dpy):
	EGLContextBase(dpy)
{
}

NS_CLOSE_GLSP_EGL()
