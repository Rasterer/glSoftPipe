#include "ScreenMapper.h"
#include "DataFlow.h"
#include "DrawEngine.h"

#include "iostream" //jzb

using glm::vec4;

ScreenMapper::ScreenMapper():
	PipeStage("Viewport Transform", DrawEngine::getDrawEngine())
{
}

void ScreenMapper::setViewPort(int x, int y, int w, int h)
{
	mViewPort.x = x;
	mViewPort.y = y;
	mViewPort.w = w;
	mViewPort.h = h;
}

void ScreenMapper::getViewPort(int &x, int &y, int &w, int &h)
{
	x = mViewPort.x;
	y = mViewPort.y;
	w = mViewPort.w;
	h = mViewPort.h;
}

void ScreenMapper::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	viewportTransform(bat);

	getNextStage()->emit(bat);
}

void ScreenMapper::viewportTransform(Batch *bat)
{
#if PRIMITIVE_OWNS_VERTICES
	PrimBatch &in = bat->mPrims;

	for(PrimBatch::iterator it = in.begin(); it != in.end(); ++it)
	{
		for(size_t i = 0; i < 3; ++i)
		{
			vec4 &pos = it->mVert[i].position();
			pos.x = (pos.x + 1.0f) * mViewPort.w / 2.0;
			pos.y = (pos.y + 1.0f) * mViewPort.h / 2.0;

			std::cout << "jzb: viewport x " << pos.x << " y " << pos.y << " z " << pos.z << std::endl;
		}
	}
#endif
}

void ScreenMapper::finalize()
{
}
