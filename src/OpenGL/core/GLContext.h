#pragma once

#include "BufferObject.h"
#include "VertexArrayObject.h"
#include "Texture.h"
#include "Shader.h"
#include "FrameBufferObject.h"


namespace glsp {

extern GLContext *g_GC;

#define __GET_CONTEXT()		\
		GLContext *gc = g_GC;

#define __SET_CONTEXT(gc)	\
		::glsp::setCurrentContext(gc);

#define EMIT_FLAG_VIEWPORT (1 << 0)

class DrawEngine;

struct GLViewport
{
	// Used to store the params from user
	int   x, y;
	int   width, height;
	float zNear, zFar;

	// Used for internal viewport transform
	// TODO: depth range
	float   xCenter, yCenter;
	float   xScale,  yScale;
};

struct ClearState
{
	float  red, green, blue, alpha;
	double depth;
	int    stencil;
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
	ClearState mClearState;
};

// GLContext needs to be accessed by most components.
// So make its data members all public here.
class GLContext
{
public:
	GLContext(int major, int minor, DrawEngine &de);
	~GLContext();

	void applyViewport(int x, int y, int width, int height);

public:
	BufferObjectMachine       mBOM;
	VAOMachine                mVAOM;
	ProgramMachine            mPM;
	TextureMachine            mTM;
	FrameBufferObjectMachine  mFBOM;

	GLStateMachine      mState;
	unsigned int        mEmitFlag;

	RenderTarget        mRT;

	DrawEngine		   &mDE;

	bool				mbInFrame;

	void *mpEGLContext;

	int   mVersionMajor;
	int   mVersionMinor;

private:
	void initGC();
};

GLContext* getCurrentContext();
void MakeCurrent(void *gc);

} // namespace glsp
