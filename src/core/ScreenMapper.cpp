#include "ScreenMapper.h"

int ScreenMapper::viewPort(int x, int y, int w, int h)
{
	mViewPort.x = x;
	mViewPort.y = y;
	mViewPort.w = w;
	mViewPort.h = h;
}

int ScreenMapper::setupInput(size_t vertexCount, vec4 *posInNDC)
{
	mVertexCount = vertexCount;
	mPosInNDC = posInNDC;
	return 0;
}

int ScreenMapper::mapNDCToScreen()
{
	for(int i = 0; i < mVertexCount; i++)
	{
		mPosInNDC[i].x = (mPosInNDC[i].x + 1.0) * mViewPort.w / 2.0;
		mPosInNDC[i].y = (mPosInNDC[i].y + 1.0) * mViewPort.h / 2.0;
	}

	return 0;
}
