#pragma once

#include "DrawEngineExport.h"


namespace glsp {

class INativeWindowManager
{
public:
	static INativeWindowManager* get();
	static void release();

	INativeWindowManager() = default;
	virtual ~INativeWindowManager() = default;

	bool InitDrawEngine();
	void FinishFrame();

	virtual bool NWMCreateWindow(int width, int height, const char *name) = 0;
	virtual void NWMDestroyWindow() = 0;

private:
	virtual void GetWindowInfo(NWMWindowInfo *win_info) = 0;
	virtual bool DisplayFrame(NWMBufferToDisplay *buf) = 0;

	static INativeWindowManager *sNWM;

	static void NWMGetWindowInfo(NWMWindowInfo *win_info);
};

INativeWindowManager* CreateNativeWindowManager();

} // namespace glsp
