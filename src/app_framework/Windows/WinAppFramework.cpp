#include "WinAppFramework.h"
#include <cstdio>

namespace glsp {


bool WinAppFramework::AFWCreateWindow(int w, int h, const char *name)
{
	mAppInstance = ::GetModuleHandle(nullptr);

	return mNWM->NWMCreateWindow(w, h, name);
}

IAppFramework* OpenAppFramework()
{
	return new WinAppFramework();
}

} // namespace glsp
