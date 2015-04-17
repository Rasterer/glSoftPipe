#pragma once

#include <vector>

NS_OPEN_GLSP_EGL()

enum ResourceType
{
	EGL_CONTEXT_TYPE = 0,
	EGL_SURFACE_TYPE = 1,
	EGL_MAX_RESOURCE = 2
};

class EGLResourceBase
{
public:
	EGLResourceBase(EGLDisplayBase &dpy, ResourceType eType):
		mDisplay(dpy),
		mType(eType)
	{
	}
		
	virtual ~EGLResourceBase();

	ResourceType    getResourceType() const { return mType; }
	EGLDisplayBase& getDisplay()      const { return mDisplay; }

private:
	const EGLDisplayBase  &mDisplay;
	const ResourceType     mType
};

struct IEGLBridge
{
	void* (*createGC)(void *EglCtx, int major, int minor);
	bool  (*swapBuffers)();
	bool  (*getBuffers)(void *EglCtx, BufferHandle *handle);
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

	bool isDisplayMatch(void *nativeDpy, EGLenum platformType) const
	{
		return (platformType == mNativePlatform) &&
				  (nativeDpy == mNativeDisplay);
	}

	void*   getNativeDisplay() const { return mNativeDisplay; }
	IEGLBridge* getEGLBridge() const { return &mBridge; }

private:
	typedef std::vector<EGLResourceBase *> EGLResourceList;

	EGLResourceList mResourceList[EGL_MAX_RESOURCE];

	IEGLBridge mBridge;

	EGLContextBase *mCurrentContext;

	EGLint mMajorVersion;
	EGLint mMinorVersion;

	void   *mNativeDisplay;
	EGLenum mNativePlatform;
};

NS_CLOSE_GLSP_EGL()
