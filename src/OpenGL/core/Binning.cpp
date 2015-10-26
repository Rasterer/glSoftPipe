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
	GLContext   *gc = bat->mDC->gc;

	for (Primitive &prim: bat->mPrims)
	{
		if (prim.mAreaReciprocal > 0.0f)
		{
			const glm::vec4 &v0 = prim.mVert[0];
			const glm::vec4 &v1 = prim.mVert[1];
			const glm::vec4 &v2 = prim.mVert[2];
		}
		else
		{
			const glm::vec4 &v0 = prim.mVert[2];
			const glm::vec4 &v1 = prim.mVert[1];
			const glm::vec4 &v2 = prim.mVert[0];
		}
		const int x0 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[0].position().x);
		const int y0 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[0].position().y);

		const int x1 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[1].position().x);
		const int y1 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[1].position().y);

		const int x2 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[2].position().x);
		const int y2 = fixedpoint_cast<FIXED_POINT4>(prim.mVert[2].position().y);

		const int x1x0 = x1 - x0;
		const int x2x1 = x2 - x1;
		const int x0x2 = x0 - x2;

		const int y0y1 = y0 - y1;
		const int y1y2 = y1 - y2;
		const int y2y0 = y2 - y0;

		int C0 = x0 * y1 - x1 * y0;
		int C1 = x1 * y2 - x2 * y1;
		int C2 = x2 * y0 - x0 * y2;

		if (y0y1 > 0 || y0y1 == 0 && x1x0 < 0)  C0++;
		if (y1y2 > 0 || y1y2 == 0 && x2x1 < 0)  C1++;
		if (y2y0 > 0 || y2y0 == 0 && x0x2 < 0)  C2++;

		int xmin = ROUND_DOWN(std::min(x0, x1, x2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);
		int ymin = ROUND_DOWN(std::min(y0, y1, y2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);
		int xmax = ROUND_UP(std::max(x0, x1, x2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);
		int ymax = ROUND_UP(std::max(y0, y1, y2) >> FIXED_POINT4_SHIFT, MACRO_TILE_SIZE);

		xmin = clamp(xmin, 0, gc->mRT.width  - 1);
		xmax = clamp(xmax, 0, gc->mRT.width  - 1);
		ymin = clamp(ymin, 0, gc->mRT.height - 1);
		ymax = clamp(ymax, 0, gc->mRT.height - 1);

		for (int y = ymin; y < ymax; y += MACRO_TILE_SIZE)
		{
			for (int x = xmin; x < xmax; x += MACRO_TILE_SIZE)
			{
				y0y1 * x + x1x0 * y + C0;
			}
		}
	}
}

NS_CLOSE_GLSP_OGL()