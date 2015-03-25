#pragma once

#include "BufferObject.h"

#define __GET_CONTEXT()		\
	GLContext *gc = (GLContext *)getCurrentContext();

#define __SET_CONTEXT(gc)	\
	setCurrentContext(gc);

void initTLS();
void deinitTLS();
void setCurrentContext();
void *getCurrentContext();
bool CreateContext();
bool DestroyContext();
bool MakeCurrent(GLContext *gc);

class GLContext
{
public:
	GLContext();
	BufferObjectMachine	*mBOM;
};
