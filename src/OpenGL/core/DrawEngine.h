#pragma once

#include "DrawEngineExport.h"
#include "DataFlow.h"
#include "Texture.h"


namespace glsp {

class GLContext;
class VertexFetcher;
class PrimitiveAssembler;
class Clipper;
class PerspectiveDivider;
class ScreenMapper;
class FaceCuller;
class Binning;
class TBDR;
class PipeStage;
class Interpolater;
class OwnershipTester;
class ScissorTester;
class StencilTester;
class ZTester;
class Blender;
class Dither;
class FBWriter;
class GeometryStage;
class RasterizationStage;
class FragmentShader;

// Hold raster states for deferred rendering support.
struct RasterStates
{
	struct {
		int mIsDepthTestEnable : 1;
		int mIsBlendEnable     : 1;
		int mIsDepthOnly       : 1;
	};

	uint32_t mDrawID;
	FragmentShader 		*mFS;
	Texture				*mTextures[MAX_TEXTURE_UNITS];
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
	const void 		*mIndices;
	GLContext 		*gc;
	RasterStates    *mRasterStates;
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
	friend class TBDR;

	void init();
	void SetNativeWindowInfo(NWMWindowInfo &win_info);
	bool validateState(DrawContext *dc);
	void prepareToDraw();
	void emit(DrawContext *dc);
	bool SwapBuffers(NWMBufferToDisplay *buf);
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

	RasterizationStage* getRastStage() const { return mRast; }
	Clipper& GetClipper() const { return *mClipper; }

	GLContext* GetGLContext() const { return mGLContext; }

	void PerformClear(unsigned int mask);

	void Flush(bool swap_buffer);

protected:
	DrawEngine();
	~DrawEngine();


private:
	void beginFrame(GLContext *dc);
	void initPipeline();
	void linkGeomertryPipeStages();
	void linkRasterizerPipeStages();

	// Use pointer member because there may be serveral impls of this components.
	// And we may need to switch between those dynamically.
	VertexFetcher 			*mVertexFetcher;
	PrimitiveAssembler 		*mPrimAsbl;
	Clipper 				*mClipper;
	PerspectiveDivider 		*mDivider;
	ScreenMapper 			*mMapper;
	FaceCuller 				*mCuller;
	Binning  				*mBinning;

	TBDR                    *mTBDR;
	Interpolater            *mInterpolater;
	OwnershipTester         *mOwnershipTest;
	ScissorTester           *mScissorTest;
	StencilTester           *mStencilTest;
	ZTester                 *mDepthTest;
	Blender                 *mBlender;
	Dither                  *mDither;
	FBWriter                *mFBWriter;

	// Geometry/Rasterization wrapper
	GeometryStage           *mGeometry;
	RasterizationStage      *mRast;

	PipeStage			   *mFirstStage;

	GLContext              *mGLContext;
	uint32_t                mDrawCount;
};

} // namespace glsp
