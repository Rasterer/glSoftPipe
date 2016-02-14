#include "GLContext.h"

#include <cstring>

#include "DrawEngine.h"
#include "Clipper.h"
#include "glsp_debug.h"
#include "khronos/GL/glspcorearb.h"


namespace glsp {

GLAPI void APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	__GET_CONTEXT();
	GLViewport &vp = gc->mState.mViewport;

	if (vp.x == y && vp.y == y &&
		vp.width == width && vp.height == height)
		return;
	else
		gc->applyViewport(x, y, width, height);

	return;
}

GLAPI void APIENTRY glEnable (GLenum cap)
{
	__GET_CONTEXT();

	// TODO: impl other options
	switch (cap)
	{
		case GL_DEPTH_TEST:
		{
			gc->mState.mEnables |= GLSP_DEPTH_TEST;
			break;
		}
		case GL_CULL_FACE:
		{
			gc->mState.mEnables |= GLSP_CULL_FACE;
			break;
		}
		default:
		{
			GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "unknown cap\n");
		}
	}
}

GLContext *g_GC = nullptr;

GLContext* getCurrentContext()
{
	return g_GC;
}

void MakeCurrent(void *gc)
{
	g_GC = static_cast<GLContext *>(gc);
}

GLContext::GLContext(int major, int minor, DrawEngine &de):
	mEmitFlag(0),
	mDE(de),
	mbInFrame(false),
	mVersionMajor(major),
	mVersionMinor(minor)
{
	initGC();
}

GLContext::~GLContext()
{
	if (g_GC == this)
		g_GC = nullptr;
}

void GLContext::initGC()
{
	applyViewport(0, 0, 0, 0);

	mState.mClearState.red     = 0.0f;
	mState.mClearState.green   = 0.0f;
	mState.mClearState.blue    = 0.0f;
	mState.mClearState.alpha   = 0.0f;
	mState.mClearState.depth   = 1.0;
	mState.mClearState.stencil = 0;

	mState.mEnables    = 0;

	memset(&mRT, 0, sizeof(mRT));
}

void GLContext::applyViewport(int x, int y, int width, int height)
{
	GLViewport &vp = mState.mViewport;

	vp.x      = x;
	vp.y      = y;
	vp.width  = width;
	vp.height = height;

	vp.xScale = vp.width  / 2.0f;
	vp.yScale = vp.height / 2.0f;

	vp.xCenter = vp.x + vp.xScale;
	vp.yCenter = vp.y + vp.yScale;

	DrawEngine::getDrawEngine().GetClipper().ComputeGuardband((float)width, (float)height);
}


} // namespace glsp
