#pragma once

#include "BufferObject.h"
#include "VertexArrayObject.h"
#include "Shader.h"
#include "common/glsp_defs.h"


NS_OPEN_GLSP_OGL()

#define __GET_CONTEXT()		\
		glsp::ogl::GLContext *gc = glsp::ogl::getCurrentContext();

#define __SET_CONTEXT(gc)	\
		glsp::ogl::setCurrentContext(gc);

#define EMIT_FLAG_VIEWPORT (1 << 0)

class DrawEngine;

struct GLViewport
{
	int   x, y;
	int   width, height;
	float zNear, zFar;
};

struct RenderTarget
{
	int width;
	int height;
	void  *pColorBuffer;
	float *pDepthBuffer;
	void  *pStencilBuffer;
};

// TODO: add other states
struct GLStateMachine
{
	GLViewport mViewport;
};

// GLContext needs to be accessed by most components.
// So make its data members all public here.
class GLContext
{
public:
	GLContext(void *EglCtx, int major, int minor);

	BufferObjectMachine	mBOM;
	VAOMachine			mVAOM;
	ProgramMachine		mPM;

	GLStateMachine      mState;
	unsigned int        mEmitFlag;

	RenderTarget        mRT;

	DrawEngine		   *mDE;
	DrawContext		   *mDC;

	bool				mbInFrame;

	void *mpEGLContext;

	int   mVersionMajor;
	int   mVersionMinor;

private:
	void initGC();
};

void initTLS();
void deinitTLS();
void setCurrentContext(void *gc);
GLContext *getCurrentContext();
void* CreateContext(void *EglCtx, int major, int minor);
void DestroyContext(void *gc);
bool MakeCurrent(GLContext *gc);

NS_CLOSE_GLSP_OGL()
