#include "EGLGlobal.h"

#include <algorithm>
#include "EGLDisplayBase.h"
#include "EGLDisplayX11.h"

NS_OPEN_GLSP_EGL()

// TODO: add other platforms support
EGLGlobal::EGLGlobal():
	mCurrentDisplay(NULL),
	mNativePlatform(EGL_PLATFORM_X11_KHR)
{
}

bool EGLGlobal::validateDisplay(EGLDisplayBase *dpy)
{
	EGLDisplayList::iterator it = std::find(mDisplayList.begin(), mDisplayList.end(), dpy);

	return (it != mDisplayList.end());
}

EGLDisplayBase* EGLGlobal::getDisplay(void *nativeDpy)
{
	EGLDisplayBase *pDisp = NULL;
	EGLenum platformType = getPlatformType();

	for(auto it = mDisplayList.begin();
		it != mDisplayList.end();
		it++)
	{
		if((*it)->isDisplayMatch(nativeDpy, platformType))
		{
			pDisp = *it;
			break;
		}
	}

	if(pDisp)
		return pDisp;

	switch(platformType)
	{
		case EGL_PLATFORM_X11_KHR:
			pDisp = new EGLDisplayX11(nativeDpy, platformType);
			break;

		default:
			break;
	}

	if(pDisp)
		mDisplayList.push_back(pDisp);

	return pDisp;
}

void EGLGlobal::cleanUp()
{
	for(auto it = mDisplayList.begin();
		it != mDisplayList.end();
		it++)
	{
		delete *it;
	}

	mDisplayList.clear();
}

NS_CLOSE_GLSP_EGL()
