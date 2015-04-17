#pragma once

#include <xcb/xcb.h>

#include "EGLContextBase.h"

namespace glsp {

class EGLDisplayBase;

class EGLContextX11: public EGLContextBase
{
public:
	EGLContextX11(EGLDisplayBase &dpy);
	virtual ~EGLContextX11() { }
};

} //namespace glsp
