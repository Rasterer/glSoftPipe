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

// contain serveral pipe stages
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
	PrimitiveProcessor		mPP;
	Rasterizer				mRast;
	DrawContext			   *mCtx;
};
