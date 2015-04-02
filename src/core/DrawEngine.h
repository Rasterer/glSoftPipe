#include "VertexCache.h"
#include "PrimitiveAssembler.h"
#include "Clipper.h"
#include "FaceCuller.h"
#include "PerspectiveDivider.h"
#include "ScreenMapper.h"
#include "Rasterizer.h"

class GLContext;

struct DrawContext
{
	unsigned mMode;
	int mFirst;
	int mCount;
	unsigned mIndexType;
	void *mIndices;
	GLContext *gc;
};

// Containing fixed funtion pipeline stages
// Shaders are injected from DrawContext.gc
class DrawEngine
{
public:
	void init();
	void validateState(GLContext *gc);
	void emit(DrawContext *dc, GLContext *gc);

	// accessors
	static DrawEngine  *getDrawEngine() { return &DrawEngine::DE; }
	DrawContext *getDrawContext() const { return mCtx; }
	PipeStage   *getFirstStage() const { return mFirstStage; }

	// mutators
	void setDC(DrawContext *dc) { mCtx = dc; }
	void setFirstStage(PipeStage *stage) { mFirstStage = stage; }

private:
	DrawEngine();

private:
	VertexCachedAssembler	mAsbl;
	PrimitiveAssembler		mPrimAsbl;
	Clipper					mClipper;
	PerspectiveDivider		mDivider;
	ScreenMapper			mMapper;
	FaceCuller				mCuller;
	Rasterizer				mRast;
	DrawContext			   *mCtx;
	PipeStage			   *mFirstStage;

	// implicit singleton
	static DrawEngine		DE;
};
