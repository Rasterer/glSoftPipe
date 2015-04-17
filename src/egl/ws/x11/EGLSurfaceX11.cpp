#include "EGLSurfaceX11.h"

NS_OPEN_GLSP_EGL()

EGLSurfaceX11::EGLSurfaceX11(EGLDisplayBase &dpy, EGLenum type):
	EGLSurfaceBase(dpy, type)
{
}

EGLSurfaceX11::initSurface(EGLDisplayX11 *dpy)
{
	xcb_connection_t  *c = dpy->getXCBConnection();
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

}

NS_CLOSE_GLSP_EGL()
