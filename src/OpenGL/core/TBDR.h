#pragma once

#include "Rasterizer.h"
#include "PipeStage.h"
#include "glsp_defs.h"


// 64x64
#define MACRO_TILE_SIZE           64
#define MACRO_TILE_SIZE_SHIFT     6

// 8x8
#define MICRO_TILE_SIZE           8
#define MICRO_TILE_SIZE_SHIFT     3

#define MICRO_TILES_IN_MACRO_TILE (MACRO_TILE_SIZE / MICRO_TILE_SIZE)

// support 720p window at maximum
#define MAX_RASTERIZATION_WIDTH   2048
#define MAX_RASTERIZATION_HEIGHT  1024

#define MAX_TILES_IN_WIDTH        (MAX_RASTERIZATION_WIDTH  >> MACRO_TILE_SIZE_SHIFT)
#define MAX_TILES_IN_HEIGHT       (MAX_RASTERIZATION_HEIGHT >> MACRO_TILE_SIZE_SHIFT)


NS_OPEN_GLSP_OGL()

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
	/* Represent the reciprocal of the original vertex attributes
	 * (except for position, which stores the fixed point coords).
	 * Used to make rasterizer easy and fast.
	 */
	vsOutput	mAttrReciprocal;

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

	vsOutput			mGradientX;
	vsOutput			mGradientY;

	// NOTE: the order may be changed from the original order
	// in Primitive to keep counter-clockwise
	IntermVertex		mVert[3];

	// Bounding box.
	int					xmin;
	int					xmax;
	int					ymin;
	int					ymax;
	const Primitive	   &mPrim;
	// used to compute texture mipmap lambda. Refer to:
	// http://www.gamasutra.com/view/feature/3301/runtime_mipmap_filtering.php?print=1
	float a, b, c, d, e, f;
};

class TBDR: public Rasterizer
{
public:
	TBDR();
	~TBDR();

	virtual void finalize();

private:
	typedef Triangle  *PixelPrimMap[MACRO_TILE_SIZE][MACRO_TILE_SIZE];
	typedef float           ZBuffer[MACRO_TILE_SIZE][MACRO_TILE_SIZE];

	virtual void onRasterizing(DrawContext *dc);
	void ProcessMacroTile(int x, int y);
	void RenderOnePixel(Triangle *tri, int x, int y, float z);

	PixelPrimMap  *mPixelPrimMap;
	ZBuffer       *mZBuffer;
};

class PerspectiveCorrectInterpolater: public Interpolater
{
public:
	PerspectiveCorrectInterpolater() = default;
	~PerspectiveCorrectInterpolater() = default;

	// PipeStage interface
	virtual void emit(void *data);

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

NS_CLOSE_GLSP_OGL()
