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

bool INativeWindowManager::InitDrawEngine()
{
	NWMCallBacks call_backs =
	{
		&NWMGetWindowInfo,
	};

	// Create our glSoftPipe render.
	return glspCreateRender(&call_backs);
}

void INativeWindowManager::FinishFrame()
{
	NWMBufferToDisplay buf;
	glspSwapBuffers(&buf);

	DisplayFrame(&buf);
}

void INativeWindowManager::NWMGetWindowInfo(NWMWindowInfo *win_info)
{
	INativeWindowManager::get()->GetWindowInfo(win_info);
}

} // namespace glsp
