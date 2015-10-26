#include "Binning.h"

#include "utils.h"


NS_OPEN_GLSP_OGL()


Binning::Binning()
{
}

Binning::~Binning()
{
}

Binning::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(bat);

	for (Primitive &prim: bat->mPrims)
	{
		int x0 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[0].position().x);
		int y0 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[0].position().y);

		int x1 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[1].position().x);
		int y1 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[1].position().y);

		int x2 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[2].position().x);
		int y2 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[2].position().y);

		int xmin = ROUND_DOWN(std::min(x0, x1, x2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);
		int ymin = ROUND_DOWN(std::min(y0, y1, y2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);
		int xmax = ROUND_UP(std::max(x0, x1, x2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);
		int ymax = ROUND_UP(std::max(y0, y1, y2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);

		xmin = 
	}
}
