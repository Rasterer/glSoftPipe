#pragma once

#include <xcb/xcb.h>

#include "EGLContextBase.h"
#include "glsp_defs.h"


NS_OPEN_GLSP_EGL()

class EGLDisplayBase;

class EGLContextX11: public EGLContextBase
{
public:
	EGLContextX11(EGLDisplayBase &dpy);
	virtual ~EGLContextX11() { }

public:
};

NS_CLOSE_GLSP_EGL()
