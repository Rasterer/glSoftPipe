#include "VertexCache.h"
#include "PrimitiveAssembler.h"
#include "Clipper.h"
#include "FaceCuller.h"
#include "PerspectiveDivider.h"
#include "ScreenMapper.h"
#include "Rasterizer.h"

class GLContext;

struct RenderTarget
{
	int width;
	int height;
	void  *pColorBuffer;
	float *pDepthBuffer;
	void  *pStencilBuffer;
};

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

	RenderTarget mRT;
};

// DrawEngine is the abstraction of GPU pipeline, containing fixed function stages.
// Shaders are injected from DrawContext during validateState().
// It's a singleton, but can be triggered in any thread without lock protection.
// That requires the respect stage classes shall not use writable members,
// or any other global writable variables(local variables or heap alloc are qualified).
class DrawEngine
{
public:
	void init();
	bool validateState(DrawContext *dc);
	void prepareToDraw(DrawContext *dc);
	void emit(DrawContext *dc);
	void SwapBuffers(DrawContext *dc);
	void finalize();

	// accessors
	static DrawEngine  *getDrawEngine() { return &DrawEngine::DE; }
	PipeStage   *getFirstStage() const { return mFirstStage; }

	// mutators
	void setFirstStage(PipeStage *stage) { mFirstStage = stage; }

private:
	DrawEngine();
	void beginFrame(DrawContext *dc);

private:
	VertexCachedAssembler	mAsbl;
	PrimitiveAssembler		mPrimAsbl;
	Clipper					mClipper;
	PerspectiveDivider		mDivider;
	ScreenMapper			mMapper;
	FaceCuller				mCuller;
	Rasterizer				mRast;

	// TODO(done): Use member pointer may limit the multi-thread use of DrawEngine.
	// Find a more decent way
	// DrawContext			   *mCtx;
	PipeStage			   *mFirstStage;

	// implicit singleton
	static DrawEngine		DE;
};
