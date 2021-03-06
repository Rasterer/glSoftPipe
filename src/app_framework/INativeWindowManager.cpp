#include "INativeWindowManager.h"

#include "compiler.h"

namespace glsp {

INativeWindowManager* INativeWindowManager::sNWM(nullptr);


INativeWindowManager* INativeWindowManager::get()
{
	if (UNLIKELY(!sNWM))
	{
		sNWM = CreateNativeWindowManager();
	}

	return sNWM;
}

void INativeWindowManager::release()
{
	if (sNWM)
	{
		delete sNWM;
		sNWM = nullptr;
	}
}

INativeWindowManager::INativeWindowManager():
	mAppFramework(nullptr),
	mWNDWidth(0),
	mWNDHeight(0),
	mWNDName(nullptr)
{
}

bool INativeWindowManager::InitDrawEngine()
{
	// Create our glSoftPipe render.
	return glspCreateRender();
}

void INativeWindowManager::FinishFrame()
{
	NWMBufferToDisplay buf;
	glspSwapBuffers(&buf);

	DisplayFrame(&buf);
}

bool INativeWindowManager::NWMCreateWindow(int width, int height, const char *name)
{
	mWNDWidth  = width;
	mWNDHeight = height;
	mWNDName   = name;

	// TODO: pass format
	NWMWindowInfo win_info = {width, height, 0};
	glspSetNativeWindowInfo(&win_info);

	return true;
}

} // namespace glsp
