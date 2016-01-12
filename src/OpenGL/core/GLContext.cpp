#include "GLContext.h"

#include "DrawEngine.h"
#include "Clipper.h"
#include "glsp_debug.h"


namespace glsp {
#include "khronos/GL/glcorearb.h"

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

static void setCurrentContext(void *gc)
{
	g_GC = static_cast<GLContext *>(gc);
}

GLContext* getCurrentContext()
{
	return g_GC;
}

GLContext* CreateContext(int major, int minor)
{
	GLContext *gc = new GLContext(major, minor);
	return gc;
}

void DestroyContext(void *_gc)
{
	__GET_CONTEXT();
	GLContext *_del = static_cast<GLContext *>(_gc);

	if(gc == _del)
		__SET_CONTEXT(nullptr);

	delete _del;
}

void MakeCurrent(void *gc)
{
	setCurrentContext(gc);
}

GLContext::GLContext(int major, int minor):
	mEmitFlag(0),
	mbInFrame(false),
	mVersionMajor(major),
	mVersionMinor(minor)
{
	initGC();
}

GLContext::~GLContext()
{
	if (mRT.pDepthBuffer)
	{
		free(mRT.pDepthBuffer);
	}

	if (mRT.pStencilBuffer)
	{
		free(mRT.pStencilBuffer);
	}
}

void GLContext::initGC()
{
	applyViewport(0, 0, 0, 0);

	mState.mEnables    = 0;

	mRT.pColorBuffer   = nullptr;
	mRT.pDepthBuffer   = nullptr;
	mRT.pStencilBuffer = nullptr;
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
