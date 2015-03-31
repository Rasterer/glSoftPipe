// Mainly fixed functions

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
	DrawEngine();
	void init();
	void validateState(GLContext *gc);
	void emit(DrawContext *dc, GLContext *gc);
	inline DrawContext *getDC()
	{
		return mCtx;
	}

private:
	VertexCachedAssembler	mAsbl;
	PrimitiveAssembler		mPrimAsbl;
	Clipper					mClipper;
	FaceCuller				mCuller;
	PerspectiveDivider		mDivider;
	ViewportMapper			mMapper;
	Rasterizer				mRast;
	DrawContext			   *mCtx;
};
