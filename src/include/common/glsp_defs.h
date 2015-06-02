#pragma once

#ifndef GLSP_UNREFERENCED_PARAM
#define GLSP_UNREFERENCED_PARAM(param)	((void)param)
#endif

#define NS_OPEN_GLSP_EGL()	\
namespace glsp {			\
namespace egl {

#define NS_CLOSE_GLSP_EGL()	\
} /* namespace egl  */		\
} /* namespace glsp */		

#define NS_OPEN_GLSP_OGL()	\
namespace glsp {			\
namespace ogl {

#define NS_CLOSE_GLSP_OGL()	\
} /* namespace ogl  */		\
} /* namespace glsp */		
