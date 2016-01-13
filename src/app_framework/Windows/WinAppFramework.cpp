#include "WinAppFramework.h"
#include <cstdio>
namespace glsp {


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
            Render();
        }
    }
}

LRESULT WinAppFramework::WinWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
/*    case WM_PAINT:
        hdc = ::BeginPaint(hWnd, &ps);
        ::EndPaint(hWnd, &ps);
        break;*/
    case WM_KEYDOWN:
    {
        onKeyPressed(wParam);
        break;
    }

    default:
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

IAppFramework* OpenAppFramework()
{
	return new WinAppFramework();
}

} // namespace glsp
