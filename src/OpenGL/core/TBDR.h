#pragma once

#include "Rasterizer.h"
#include "PipeStage.h"
#include "utils.h"


// 64x64
#define MACRO_TILE_SIZE           64
#define MACRO_TILE_SIZE_SHIFT     6

// 8x8
#define MICRO_TILE_SIZE           8
#define MICRO_TILE_SIZE_SHIFT     3

#define MICRO_TILES_IN_MACRO_TILE (MACRO_TILE_SIZE / MICRO_TILE_SIZE)

// support 720p window at maximum
#define MAX_RASTERIZATION_WIDTH   2048
#define MAX_RASTERIZATION_HEIGHT  2048

#define MAX_TILES_IN_WIDTH        (MAX_RASTERIZATION_WIDTH  >> MACRO_TILE_SIZE_SHIFT)
#define MAX_TILES_IN_HEIGHT       (MAX_RASTERIZATION_HEIGHT >> MACRO_TILE_SIZE_SHIFT)

#define RAST_SUBPIXEL_BITS  FIXED_POINT4_SHIFT
#define RAST_SUBPIXELS      FIXED_POINT4


namespace glsp {

class DrawEngine;
class Batch;
class Triangle;
using std::vector;

class Binning: public PipeStage
{
public:
	Binning();
	virtual ~Binning();

	virtual void emit(void *data);
	virtual void finalize();

private:
	void onBinning(Batch *bat);
};


class IntermVertex
{
public:
	// Used to calculate edge equation.
	int			mFactorA;
	int			mFactorB;
	int			mFactorC;
};

class Triangle
{
public:
	Triangle(Primitive &prim);
	~Triangle() = default;

	// Plane equation, used for fast computation of perspective corrected barycentric coordinates.
	float				mPCBCOnW0GradientX;
	float				mPCBCOnW0GradientY;
	float				mPCBCOnW0AtOrigin;

	float				mPCBCOnW1GradientX;
	float				mPCBCOnW1GradientY;
	float				mPCBCOnW1AtOrigin;

	float				mWRecipGradientX;
	float				mWRecipGradientY;
	float				mWRecipAtOrigin;

	// Depth plane equation, used for fast depth interpolation.
	float				mZGradientX;
	float				mZGradientY;
	float				mZAtOrigin;

	// Attributes plane equation, used for fast attributes interpolation.
	vsOutput			mAttrPlaneEquationA;
	vsOutput			mAttrPlaneEquationB;

	// NOTE: the order may be changed from the original order
	// in Primitive to keep counter-clockwise
	IntermVertex		mVert[3];

	// Bounding box.
	int					xmin;
	int					xmax;
	int					ymin;
	int					ymax;
	const Primitive	   &mPrim;

	// Store the vert2(in counter-clockwise) for later attribute plane equation.
	const vsOutput     *mVert2;

#if 0
	// used to compute texture mipmap lambda. Refer to:
	// http://www.gamasutra.com/view/feature/3301/runtime_mipmap_filtering.php?print=1
	float a, b, c, d, e, f;
#endif
};

class TBDR: public Rasterizer
{
public:
	TBDR(DrawEngine &de);
	~TBDR();

	virtual void finalize();

	void FlushDisplayLists(bool swap_buffer);
	void SetDepthClearFlag() { mDepthClearFlag = true; }

private:
	typedef Triangle  *PixelPrimMap[MACRO_TILE_SIZE][MACRO_TILE_SIZE];
	typedef float           ZBuffer[MACRO_TILE_SIZE][MACRO_TILE_SIZE];

	virtual void onRasterizing();
	void ProcessMacroTile(int x, int y);
	void RenderOnePixel(Triangle *tri, int x, int y, float z);
	void RenderQuadPixels(PixelPrimMap pp_map, int x, int y, /* float z, */ int i, int j);
	inline void RenderQuadPixelsInOneTriangle(Triangle *tri, int coverage_mask, int x, int y, int i, int j);

	DrawEngine    &mDE;
	PixelPrimMap  *mPixelPrimMap;
	ZBuffer       *mZBuffer;

	bool           mDepthClearFlag;

	// Used to optimize the depth buffer store.
	// In the case where flush is triggered by swap buffer,
	// we can eliminate the depth buffer store.
	// This is the very zero depth store implemented in HW.
	bool           mFlushTriggerBySwapBuffer;
};

class PerspectiveCorrectInterpolater: public Interpolater
{
public:
	PerspectiveCorrectInterpolater() = default;
	~PerspectiveCorrectInterpolater() = default;

	// PipeStage interface
	virtual void emit(void *data);

	static void onInterpolatingSISD(void *data);
	static void onInterpolatingSIMD(void *data);

	virtual void onInterpolating(const fsInput &in,
								 const fsInput &gradX,
								 const fsInput &gradY,
								 float stepX, float stepY,
								 fsInput &out);

	// step 1 in X or Y axis
	virtual void onInterpolating(const fsInput &in,
								 const fsInput &grad,
								 float x,
								 fsInput &out);
};

} // namespace glsp
