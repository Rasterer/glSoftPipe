#include <windows.h>

#include "IAppFramework.h"

namespace glsp {

class WinAppFramework: public IAppFramework
{
public:
    WinAppFramework() = default;
    virtual ~WinAppFramework() = default;

    //virtual void* GetAppHandle(GlspApp *app);
    virtual void EnterLoop();
    //virtual void EventDispatch();

private:
    virtual bool AFWCreateWindow(int w, int h, const char *name);

    HINSTANCE mAppInstance;
};

bool WinAppFramework::AFWCreateWindow(int w, int h, const char *name)
{
    if (w <= 0 || h <= 0 || !name)
    	return false;

	mAppInstance = ::GetModuleHandle(nullptr);

	return mNWM->NWMCreateWindow(w, h, name);
}

void WinAppFramework::EnterLoop()
{
    // Main message loop
    MSG msg = {0};
    while(WM_QUIT != msg.message)
    {
        if(::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg );
            ::DispatchMessage( &msg );
        }
        else
        {
            mApp->Render();
        }
    }
}

IAppFramework* OpenAppFramework()
{
	return new WinAppFramework();
}

} // namespace glsp
