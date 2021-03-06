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
    //virtual void EventDispatch();

private:
    virtual bool AFWCreateWindow(int w, int h, const char *name);

    HINSTANCE mAppInstance;
};


} // namespace glsp
