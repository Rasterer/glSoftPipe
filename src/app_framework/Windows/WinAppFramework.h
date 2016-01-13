#pragma once

#include "IAppFramework.h"

#include <windows.h>

namespace glsp {

class WinAppFramework: public IAppFramework
{
public:
    WinAppFramework() = default;
    virtual ~WinAppFramework() = default;

    //virtual void* GetAppHandle(GlspApp *app);
    virtual void EnterLoop();
    //virtual void EventDispatch();

    LRESULT WinWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    virtual bool AFWCreateWindow(int w, int h, const char *name);

    HINSTANCE mAppInstance;
};


} // namespace glsp
