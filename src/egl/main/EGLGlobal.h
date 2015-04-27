#pragma once

#include <vector>
#include <boost/serialization/singleton.hpp>

#include <khronos/EGL/egl.h>
#include <khronos/EGL/eglext.h>
#include <common/glsp_defs.h>

NS_OPEN_GLSP_EGL()

class EGLGlobal;
class EGLDisplayBase;

typedef boost::serialization::singleton<EGLGlobal> EGLGlobalSingleton;

class EGLGlobal
{
public:
	// singleton accessor
	static EGLGlobal& getEGLGloabl()
	{
		return EGLGlobalSingleton::get_mutable_instance();
	}

	bool validateDisplay(EGLDisplayBase *dpy);
	EGLDisplayBase* getDisplay(void *nativeDpy);
	void cleanUp();

	EGLenum getPlatformType() const { return mNativePlatform; }

protected:
	EGLGlobal();
	~EGLGlobal() {}

private:
	typedef std::vector<EGLDisplayBase *> EGLDisplayList;

	EGLDisplayList mDisplayList;
	EGLDisplayBase* mCurrentDisplay;

	EGLenum mNativePlatform;
};

NS_CLOSE_GLSP_EGL()
