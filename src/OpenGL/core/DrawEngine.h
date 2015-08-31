#pragma once

#include <common/glsp_defs.h>


namespace glsp {
	struct IEGLBridge;
}

NS_OPEN_GLSP_OGL()

class GLContext;
class VertexFetcher;
class PrimitiveAssembler;
class Clipper;
class PerspectiveDivider;
class ScreenMapper;
class FaceCuller;
class RasterizerWrapper;
class PipeStage;

namespace glsp {
	struct IEGLBridge;
}

struct DrawContext
{
	enum DrawType
	{
		kArrayDraw = 0,
		kElementDraw
	};
	unsigned mMode;
	int mFirst;
	int mCount;
	unsigned mIndexSize;
	DrawType mDrawType;
	const void *mIndices;
	GLContext *gc;
};

/*
 * DrawEngine is the abstraction of GPU pipeline, containing fixed function stages.
 * Shaders are injected from DrawContext during validateState().
 * It's a singleton, but can be triggered in any thread without lock protection.
 * That requires the respect stage classes shall not use writable members,
 * or any other global writable variables(local variables or heap alloc are qualified).
 */
class DrawEngine
{
public:
	void init(void *dpy, IEGLBridge *bridge);
	bool validateState(DrawContext *dc);
	void prepareToDraw(DrawContext *dc);
	void emit(DrawContext *dc);
	bool SwapBuffers();
	void finalize();

	// accessors
	static DrawEngine& getDrawEngine()
	{
		static DrawEngine instance;
		return instance;
	}

	PipeStage* getFirstStage() const { return mFirstStage; }

	// mutators
	void setFirstStage(PipeStage *stage) { mFirstStage = stage; }

protected:
	DrawEngine();
	~DrawEngine();


private:
	void beginFrame(GLContext *dc);
	void initPipeline();
	void linkPipeStages(GLContext *gc);

	// Use pointer member because there may be serveral impls of this components.
	// And we may need to switch between those dynamically.
	VertexFetcher 			*mVertexFetcher;
	PrimitiveAssembler 		*mPrimAsbl;
	Clipper 				*mClipper;
	PerspectiveDivider 		*mDivider;
	ScreenMapper 			*mMapper;
	FaceCuller 				*mCuller;
	RasterizerWrapper 		*mRast;

	// TODO(done): Use member pointer may limit the multi-thread use of DrawEngine.
	// Find a better way
	// DrawContext			   *mCtx;
	PipeStage			   *mFirstStage;

	// EGL related data member
	void             *mpEGLDisplay;
	IEGLBridge *mpBridge;
};

NS_CLOSE_GLSP_OGL()