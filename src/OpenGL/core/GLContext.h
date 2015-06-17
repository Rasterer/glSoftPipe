#pragma once

#include "BufferObject.h"
#include "VertexArrayObject.h"
#include "Texture.h"
#include "Shader.h"
#include "common/glsp_defs.h"


NS_OPEN_GLSP_OGL()

#define __GET_CONTEXT()		\
		::glsp::ogl::GLContext *gc = ::glsp::ogl::getCurrentContext();

#define __SET_CONTEXT(gc)	\
		::glsp::ogl::setCurrentContext(gc);

#define EMIT_FLAG_VIEWPORT (1 << 0)

class DrawEngine;

struct GLViewport
{
	// Used to store the params from user
	int   x, y;
	int   width, height;
	float zNear, zFar;

	// Used for internal viewport transform
	// FIXME: Now use fixed point, need use float point instead?
	// TODO: depth range
	int   xCenter, yCenter;
	int   xScale, yScale;
};

struct RenderTarget
{
	int width;
	int height;
	void  *pColorBuffer;
	float *pDepthBuffer;
	void  *pStencilBuffer;
};


/* NOTE:
 * No alpha test now in core profile.
 * Replaced by discard instruction in fragment shader.
 */

#define GLSP_CULL_FACE				(1 << 0)
#define GLSP_MULTISAMPLE			(1 << 1)
#define GLSP_SCISSOR_TEST			(1 << 2)
#define GLSP_STENCIL_TEST			(1 << 3)
#define GLSP_DEPTH_TEST				(1 << 4)
#define GLSP_BLEND					(1 << 5)
#define GLSP_DITHER					(1 << 6)

// TODO: add other states
struct GLStateMachine
{
	int        mEnables;
	GLViewport mViewport;
};

// GLContext needs to be accessed by most components.
// So make its data members all public here.
class GLContext
{
public:
	GLContext(void *EglCtx, int major, int minor);

	void applyViewport(int x, int y, int width, int height);

public:
	BufferObjectMachine       mBOM;
	VAOMachine                mVAOM;
	ProgramMachine            mPM;
	TextureMachine            mTM;

	// TODO: impl FBO
	//FrameBufferObjectMachine  mFBM;

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
GLContext *getCurrentContext();
void* CreateContext(void *EglCtx, int major, int minor);
void DestroyContext(void *gc);
void MakeCurrent(void *gc);

NS_CLOSE_GLSP_OGL()