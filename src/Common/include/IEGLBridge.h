#pragma once

namespace glsp {

// Bidi interfaces between egl and ogl.
struct IEGLBridge
{
	// egl -> ogl
	void* (*createGC)(void *EglCtx, int major, int minor);
	void  (*destroyGC)(void *gc);
	void  (*makeCurrent)(void *gc);
	bool  (*swapBuffers)();

	// ogl -> egl
	bool  (*getBuffers)(void *EglCtx, void **addr, int *width, int *height);
};

bool iglCreateScreen(void *dpy, IEGLBridge *bridge);

} //namespace glsp