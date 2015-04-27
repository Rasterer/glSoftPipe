#include "khronos/EGL/egl.h"
#include "EGLGlobal.h"
#include "EGLDisplayBase.h"
#include "EGLContextBase.h"
#include "EGLSurfaceBase.h"
#include "EGLConfigBase.h"


using glsp::egl::EGLGlobal;
using glsp::egl::EGLDisplayBase;
using glsp::egl::EGLContextBase;
using glsp::egl::EGLSurfaceBase;
using glsp::egl::EGLConfigBase;

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay (EGLNativeDisplayType display_id)
{
	EGLDisplayBase *pDisp = EGLGlobal::getEGLGloabl().getDisplay(display_id);

	return (EGLDisplay)pDisp;
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	EGLDisplayBase *pDisp = static_cast<EGLDisplayBase *>(dpy);
	EGLGlobal      *pGlob = &EGLGlobal::getEGLGloabl();

	if(pGlob->validateDisplay(pDisp))
	{
		bool ret = pDisp->initDisplay();

		pDisp->getEGLVersion(major, minor);

		return ret;
	}
	else
	{
		return false;
	}
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	EGLDisplayBase *pDisp = static_cast<EGLDisplayBase *>(dpy);
	EGLContextBase *pCtx  = static_cast<EGLContextBase *>(share_context);
	EGLConfigBase  *pConf = static_cast<EGLConfigBase *>(config);
	EGLGlobal      *pGlob = &EGLGlobal::getEGLGloabl();

	if(!pGlob->validateDisplay(pDisp))
	{
		return (EGLContext)NULL;
	}

	EGLContextBase *ctx = pDisp->createContext(pConf, pCtx, attrib_list);

	return (EGLContext)ctx;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
	EGLDisplayBase *pDisp = static_cast<EGLDisplayBase *>(dpy);
	EGLConfigBase  *pConf = static_cast<EGLConfigBase *>(config);
	EGLGlobal      *pGlob = &EGLGlobal::getEGLGloabl();

	if(!pGlob->validateDisplay(pDisp))
	{
		return (EGLSurface)NULL;
	}

	EGLSurfaceBase *sur = pDisp->createWindowSurface(pConf, win, attrib_list);

	return (EGLSurface)sur;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	EGLDisplayBase *pDisp = static_cast<EGLDisplayBase *>(dpy);
	EGLSurfaceBase *pDraw = static_cast<EGLSurfaceBase *>(draw);
	EGLSurfaceBase *pRead = static_cast<EGLSurfaceBase *>(read);
	EGLContextBase *pCtx  = static_cast<EGLContextBase *>(ctx);
	EGLGlobal      *pGlob = &EGLGlobal::getEGLGloabl();

	if(!pGlob->validateDisplay(pDisp))
	{
		return EGL_FALSE;
	}

	pDisp->bindContext(pCtx);
	pCtx->bindSurface(pRead, pDraw);

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
	EGLDisplayBase *pDisp = static_cast<EGLDisplayBase *>(dpy);
	EGLSurfaceBase *pSur  = static_cast<EGLSurfaceBase *>(surface);
	EGLGlobal      *pGlob = &EGLGlobal::getEGLGloabl();

	if(!pGlob->validateDisplay(pDisp))
	{
		return EGL_FALSE;
	}

	return pSur->swapBuffers();
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
	EGLDisplayBase *pDisp = static_cast<EGLDisplayBase *>(dpy);
	EGLGlobal      *pGlob = &EGLGlobal::getEGLGloabl();

	if(!pGlob->validateDisplay(pDisp))
	{
		return EGL_FALSE;
	}

	pGlob->cleanUp();

	return EGL_TRUE;
}
