#pragma once

#include <vector>

#include <khronos/EGL/egl.h>
#include <khronos/EGL/eglext.h>
#include <glsp_defs.h>


NS_OPEN_GLSP_EGL()

class EGLGlobal;
class EGLDisplayBase;


class EGLGlobal
{
public:
	// singleton accessor
	static EGLGlobal& getEGLGloabl()
	{
		static EGLGlobal instance;
		return instance;
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
