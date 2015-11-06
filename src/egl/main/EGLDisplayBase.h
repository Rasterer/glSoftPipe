#pragma once

#include <vector>

#include "glsp_defs.h"
#include "IEGLBridge.h"
#include "khronos/EGL/egl.h"


NS_OPEN_GLSP_EGL()

class EGLDisplayBase;
class EGLContextBase;
class EGLSurfaceBase;
class EGLConfigBase;

enum ResourceType
{
	KEGL_CONTEXT_TYPE = 0,
	KEGL_SURFACE_TYPE = 1,
	KEGL_MAX_RESOURCE = 2
};

class EGLResourceBase
{
public:
	EGLResourceBase(EGLDisplayBase &dpy, ResourceType eType):
		mDisplay(dpy),
		mType(eType)
	{
	}
		
	virtual ~EGLResourceBase() { }

	ResourceType     getResourceType() const { return mType; }
	const EGLDisplayBase& getDisplay() const { return mDisplay; }

private:
	const EGLDisplayBase  &mDisplay;
	const ResourceType     mType;
};

class EGLDisplayBase
{
public:
	EGLDisplayBase(void *nativeDpy, EGLenum platformType);
	virtual ~EGLDisplayBase();

	virtual bool initDisplay();

	virtual EGLContextBase* createContext(
			EGLConfigBase *config,
			EGLContextBase *share_context,
			const EGLint *attrib_list) = 0;

	virtual EGLSurfaceBase* createWindowSurface(
			EGLConfigBase *config,
			EGLNativeWindowType win,
			const EGLint *attrib_list) = 0;

	virtual EGLBoolean makeCurrent(
			EGLSurfaceBase *draw,
			EGLSurfaceBase *read,
			EGLContextBase *ctx) = 0;

	bool validateResource(EGLResourceBase *res);
	void attachResource(EGLResourceBase *res);

	void bindContext(EGLContextBase *ctx);
	EGLContextBase* getCurrentContext() const { return mCurrentContext; }

	bool isDisplayMatch(void *nativeDpy, EGLenum platformType) const
	{
		return (platformType == mNativePlatform) &&
				  (nativeDpy == mNativeDisplay);
	}

	void* getNativeDisplay() const { return mNativeDisplay; }
	const IEGLBridge* getEGLBridge() const { return &mBridge; }

	void getEGLVersion(EGLint *major, EGLint *minor) const;

private:
	typedef std::vector<EGLResourceBase *> EGLResourceList;

	EGLResourceList mResourceList[KEGL_MAX_RESOURCE];

	glsp::IEGLBridge mBridge;

	EGLContextBase *mCurrentContext;

	EGLint mMajorVersion;
	EGLint mMinorVersion;

	void   *mNativeDisplay;
	EGLenum mNativePlatform;
};

NS_CLOSE_GLSP_EGL()
