#include "EGLSurfaceX11.h"

#include <cassert>

#include <X11/Xlib-xcb.h>
#include <xcb/dri2.h>
#include "EGLDisplayX11.h"


NS_OPEN_GLSP_EGL()

EGLSurfaceX11::EGLSurfaceX11(EGLDisplayBase &dpy, EGLenum type):
	EGLSurfaceBase(dpy, type),
	mCurrentBO(NULL)
{
}

bool EGLSurfaceX11::initSurface(EGLDisplayBase *dpy, EGLNativeWindowType win)
{
	// TODO: add other drawables support
	assert(mType == EGL_WINDOW_BIT);

	mXDrawable = reinterpret_cast<::xcb_drawable_t>(win);

	EGLDisplayX11   *pDisp = static_cast<EGLDisplayX11 *>(dpy);
	::xcb_connection_t  *c = pDisp->getXCBConnection();

	if(mType != EGL_PBUFFER_BIT)
	{
		::xcb_get_geometry_cookie_t cookie;
		::xcb_get_geometry_reply_t *reply;
		::xcb_generic_error_t *error;

		cookie = ::xcb_get_geometry(c, mXDrawable);
		reply = ::xcb_get_geometry_reply(c, cookie, &error);

		if(reply == NULL || error != NULL)
		{
			free(error);
			return false;
		}

		mWidth  = reply->width;
		mHeight = reply->height;
		free(reply);
	}

	::xcb_dri2_create_drawable(c, mXDrawable);
	return true;
}

bool EGLSurfaceX11::getBuffers(void **addr, int *width, int *height)
{
	::xcb_dri2_dri2_buffer_t *buffers;
	::xcb_dri2_get_buffers_reply_t *reply;
	::xcb_dri2_get_buffers_cookie_t cookie;

	unsigned attachment[] = { XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT };

	const EGLDisplayX11 *pDisp = static_cast<const EGLDisplayX11 *>(&getDisplay());

	xcb_connection_t *c = pDisp->getXCBConnection();

	cookie = ::xcb_dri2_get_buffers_unchecked(c,
											mXDrawable,
											1,1,attachment);
	reply = ::xcb_dri2_get_buffers_reply(c, cookie, NULL);
	buffers = ::xcb_dri2_get_buffers_buffers(reply);

	*width  = mWidth  = reply->width;
	*height = mHeight = reply->height;

	if(mCurrentBO)
	{
		::drm_intel_bo_unreference(mCurrentBO);
		mCurrentBO = NULL;
	}

	drm_intel_bufmgr *pBufMgr = pDisp->getDrmBufMgr();

	mCurrentBO = ::drm_intel_bo_gem_create_from_name(pBufMgr,
													 "egl_from_handle",
													 buffers->name);

	::drm_intel_gem_bo_map_gtt(mCurrentBO);

	*addr = mCurrentBO->virt;

	free(reply);

#if 0
	BufferHandle *hdl = new BufferHandle();

	hdl->attachment = buffers->attachment;
	hdl->name       = buffers->name;
	hdl->pitch      = buffers->pitch;
	hdl->cpp        = buffers->cpp;
	hdl->flags      = buffers->flags;

	*handle = hdl;
#endif

	return true;
}

bool EGLSurfaceX11::swapBuffers()
{
	xcb_dri2_swap_buffers_cookie_t cookie;
	xcb_dri2_swap_buffers_reply_t *reply;
	int long swap_count = -1;

	const EGLDisplayX11 *pDisp = static_cast<const EGLDisplayX11 *>(&getDisplay());
	xcb_connection_t *c = pDisp->getXCBConnection();

	cookie = ::xcb_dri2_swap_buffers_unchecked(c, mXDrawable, 0, 0, 0, 0, 0, 0);
	reply  = ::xcb_dri2_swap_buffers_reply(c, cookie, NULL);

	if(reply)
	{
		swap_count = (((int long)reply->swap_hi) << 32) | reply->swap_lo;
		free(reply);
	}

	if(mCurrentBO)
	{
		::drm_intel_bo_unreference(mCurrentBO);
		mCurrentBO = NULL;
	}

	return swap_count != -1;
}

EGLSurfaceX11::~EGLSurfaceX11()
{
	if(mCurrentBO)
	{
		::drm_intel_bo_unreference(mCurrentBO);
	}
}

NS_CLOSE_GLSP_EGL()
