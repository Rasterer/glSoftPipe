#pragma once

#include "DrawEngineExport.h"


namespace glsp {
class IAppFramework;

class INativeWindowManager
{
public:
	static INativeWindowManager* get();
	static void release();

	INativeWindowManager() = default;
	virtual ~INativeWindowManager() = default;

	bool InitDrawEngine();
	void FinishFrame();

	virtual bool NWMCreateWindow(int width, int height, const char *name);
	virtual void NWMDestroyWindow() = 0;

	void SetAppFramework(IAppFramework *app_framework) { mAppFramework = app_framework; }
	IAppFramework* GetAppFramework() const { return mAppFramework; }

protected:
	IAppFramework *mAppFramework;

private:
	virtual void GetWindowInfo(NWMWindowInfo *win_info) = 0;
	virtual bool DisplayFrame(NWMBufferToDisplay *buf) = 0;

	static INativeWindowManager *sNWM;
};

INativeWindowManager* CreateNativeWindowManager();

} // namespace glsp
