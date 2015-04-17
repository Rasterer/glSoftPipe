#pragma once

#include <xf86drm.h>
#include <xcb/xcb.h>

#include "khronos/EGL/egl.h"
#include "EGLDisplayBase.h"

NS_OPEN_GLSP_EGL()

class EGLContextBase;
class EGLConfigBase;
class EGLSurfaceBase;

class EGLDisplayX11: public EGLDisplayBase
{
public:
	EGLDisplayX11(void *nativeDpy, EGLenum platformType);
	virtual ~EGLDisplayX11();

	virtual bool initDisplay();

	virtual EGLContextBase* createContext(
			EGLConfigBase *config,
			EGLContextBase *share_context,
			const EGLint *attrib_list);

	virtual EGLSurfaceBase* createWindowSurface(
			EGLConfigBase *config,
			EGLNativeWindowType win,
			const EGLint *attrib_list);

	virtual EGLBoolean makeCurrent(
			EGLSurfaceBase *draw,
			EGLSurfaceBase *read,
			EGLContextBase *ctx);

	xcb_connection_t* getXCBConnection() const { return mXCBConn; }
	xcb_connection_t* getXCBScreen()     const { return mXCBScreen; }

private:
	xcb_connection_t *mXCBConn;
	xcb_screen_t     *mXCBScreen;

	int mDrmFd;
	drm_intel_bufmgr *mBufMgr;

	const size_t batch_size_ = 16 * 1024;

	int mDri2Major;
	int mDri2Minor;

	std::string mDriverName;
	std::string mDeviceName;
};

NS_CLOSE_GLSP_EGL()
