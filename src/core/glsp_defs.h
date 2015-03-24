#pragma once

#ifndef GLSP_UNREFERENCED_PARAM
#define GLSP_UNREFERENCED_PARAM(param)	((void)param)
#endif

#define __GET_CONTEXT()	(pthread_getspecific())
struct GLContext
{
	BufferObjectMachine *mBOM;
}
