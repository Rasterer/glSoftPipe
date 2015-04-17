#pragma once

#include <boost/serialization/singleton.hpp>

#include <khronos/EGL/eglext.h>

NS_OPEN_GLSP_EGL()

class EGLGlobal;

typedef singleton<EGLGlobal> EGLGlobalSingleton;

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
