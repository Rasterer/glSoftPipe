#pragma once

#include "BufferObject.h"
#include "VertexArrayObject.h"

#define __GET_CONTEXT()		\
	GLContext *gc = getCurrentContext();

#define __SET_CONTEXT(gc)	\
	setCurrentContext(gc);

class GLContext
{
public:
	GLContext();
	BufferObjectMachine	mBOM;
	VAOMachine			mVAOM;
};

void initTLS();
void deinitTLS();
void setCurrentContext();
GLContext *getCurrentContext();
GLContext * CreateContext();
bool DestroyContext();
bool MakeCurrent(GLContext *gc);

