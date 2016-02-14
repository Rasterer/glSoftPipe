#include "GLFWAppFramework.h"

#include <cstdio>

namespace glsp {


bool GLFWAppFramework::AFWCreateWindow(int w, int h, const char *name)
{
	return mNWM->NWMCreateWindow(w, h, name);
}

IAppFramework* OpenAppFramework()
{
	return new GLFWAppFramework();
}

} // namespace glsp
