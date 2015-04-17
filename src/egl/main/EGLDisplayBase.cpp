#include "EGLDisplayBase.h"

// static routines
namespace {

bool __eglGetBuffers(void *EglCtx, BufferHandle *handle)
{
	EGLContextBase *ctx  = static_cast<EGLContextBase *>(EglCtx);
	EGLSurfaceBase *draw = ctx->getDrawSurface();

	draw->getBuffers(handle);
}

} //namespace


NS_OPEN_GLSP_EGL()

EGLDisplayBase::EGLDisplayBase(void *nativeDpy, EGLenum platformType):
	mNativeDisplay(nativeDpy),
	mNativePlatform(platformType),
	mCurrentContext(NULL),
	mCurrentSurface(NULL)
{
}

bool EGLDisplayBase::initDisplay()
{
	mMajorVersion = 1;
	mMinorVersion = 5;

	mBridge.createGC = __eglGetBuffers;

	return glsp::ogl::iglCreateScreen(this, &mBridge);
}

EGLDisplayBase::~EGLDisplayBase()
{
	for(ResourceType t = EGL_CONTEXT_TYPE; t < EGL_MAX_RESOURCE; t++)
	{
		for(EGLResourceList::iterator iter = mResourceList[t].begin();
				iter != mResourceList[t].end();
				iter++)
		{
			delete *iter;
		}

		mResourceList[t].clear;
	}
}

bool EGLDisplayBase::validateResource(EGLResourceBase *res)
{
	if(!res)
		return false;

	ResourceType type = res->getResourceType();

	assert(type < EGL_MAX_RESOURCE);

	EGLResourceList::iterator iter = mResourceList[type].find(res);

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
