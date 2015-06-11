#pragma once

#include <boost/serialization/singleton.hpp>

#include "common/IEGLBridge.h"
#include "PrimitiveAssembler.h"
#include "Clipper.h"
#include "FaceCuller.h"
#include "PerspectiveDivider.h"
#include "ScreenMapper.h"


NS_OPEN_GLSP_OGL()

using boost::serialization::singleton;

class GLContext;
class VertexFetcher;
class Rasterizer;

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

// DrawEngine is the abstraction of GPU pipeline, containing fixed function stages.
// Shaders are injected from DrawContext during validateState().
// It's a singleton, but can be triggered in any thread without lock protection.
// That requires the respect stage classes shall not use writable members,
// or any other global writable variables(local variables or heap alloc are qualified).
class DrawEngine: public singleton<DrawEngine>
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
		return get_mutable_instance();
	}

	PipeStage   *getFirstStage() const { return mFirstStage; }

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
	glsp::IEGLBridge *mpBridge;
};

NS_CLOSE_GLSP_OGL()
