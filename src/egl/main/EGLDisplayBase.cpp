#include "EGLDisplayBase.h"

#include <algorithm>
#include <cassert>

#include "EGLContextBase.h"
#include "EGLSurfaceBase.h"


NS_OPEN_GLSP_EGL()

// static routines
namespace {

bool __eglGetBuffers(void *EglCtx, void **addr, int *width, int *height)
{
	EGLContextBase *ctx  = static_cast<EGLContextBase *>(EglCtx);
	EGLSurfaceBase *draw = ctx->getDrawSurface();

	return draw->getBuffers(addr, width, height);
}

} //namespace


EGLDisplayBase::EGLDisplayBase(void *nativeDpy, EGLenum platformType):
	mCurrentContext(NULL),
	mNativeDisplay(nativeDpy),
	mNativePlatform(platformType)
{
}

bool EGLDisplayBase::initDisplay()
{
	mMajorVersion = 1;
	mMinorVersion = 5;

	mBridge.getBuffers = __eglGetBuffers;

	return glsp::iglCreateScreen(this, &mBridge);
}

EGLDisplayBase::~EGLDisplayBase()
{
	for(int t = KEGL_CONTEXT_TYPE; t < KEGL_MAX_RESOURCE; t++)
	{
		for(EGLResourceList::iterator iter = mResourceList[t].begin();
				iter != mResourceList[t].end();
				iter++)
		{
			delete *iter;
		}

		mResourceList[t].clear();
	}
}

void EGLDisplayBase::bindContext(EGLContextBase *ctx)
{
	if(mCurrentContext != ctx)
	{
		mCurrentContext = ctx;
		mBridge.makeCurrent(ctx->getClientGC());
	}
}

void EGLDisplayBase::getEGLVersion(EGLint *major, EGLint *minor) const
{
	if(major)
		*major = mMajorVersion;

	if(minor)
		*minor = mMinorVersion;
}

bool EGLDisplayBase::validateResource(EGLResourceBase *res)
{
	if(!res)
		return false;

	ResourceType type = res->getResourceType();

	assert(type < KEGL_MAX_RESOURCE);

	EGLResourceList::iterator iter = std::find(mResourceList[type].begin(),
											   mResourceList[type].end(),
											   res);

	return (iter != mResourceList[type].end());
}

void EGLDisplayBase::attachResource(EGLResourceBase *res)
{
	if(!res)
	{
		ResourceType type = res->getResourceType();
		mResourceList[type].push_back(res);
	}
}

NS_CLOSE_GLSP_EGL()
