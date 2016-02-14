#pragma once

#include "IAppFramework.h"

namespace glsp {

class GLFWAppFramework: public IAppFramework
{
public:
    GLFWAppFramework() = default;
    virtual ~GLFWAppFramework() = default;

    //virtual void* GetAppHandle(GlspApp *app);
    //virtual void EventDispatch();

private:
    virtual bool AFWCreateWindow(int w, int h, const char *name);
};


} // namespace glsp
