#include "khronos/egl.h"
#include "core/GLContext.h"
#include "core/DrawEngine.h"

namespace {

pthread_key_t TLSKey;

void initTLS()
{
	pthread_key_create(&TLSKey, NULL);
}

void deinitTLS()
{
	pthread_key_delete(TLSKey);
}

struct EGLGlobal
{
	EGLDisplayBase *disp[MAX_PLATFORM_NUM];
} gEGLGlobal;

} //namespace

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay (EGLNativeDisplayType display_id)
{
	return (EGLDisplay)0;
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	GLSP_UNREFERENCED_PARAM(dpy);

	initTLS();

	if(!major)
		*major = 1;

	if(!minor)
		*major = 5;

	return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	// TODO: utilize this params
	GLSP_UNREFERENCED_PARAM(dpy);
	GLSP_UNREFERENCED_PARAM(config);
	GLSP_UNREFERENCED_PARAM(share_context);
	GLSP_UNREFERENCED_PARAM(attrib_list);

	return CreateContext();
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	GLSP_UNREFERENCED_PARAM(dpy);
	GLSP_UNREFERENCED_PARAM(draw);
	GLSP_UNREFERENCED_PARAM(read);

	MakeCurrent(static_cast<GLContext *>(ctx));

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
	GLSP_UNREFERENCED_PARAM(dpy);
	GLSP_UNREFERENCED_PARAM(surface);

	SwapBuffers();

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
	GLSP_UNREFERENCED_PARAM(dpy);

	deinitTLS();

	return EGL_TRUE;
}
