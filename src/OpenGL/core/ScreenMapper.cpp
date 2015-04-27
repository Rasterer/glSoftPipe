#include "ScreenMapper.h"
#include "DataFlow.h"
#include "DrawEngine.h"


NS_OPEN_GLSP_OGL()

using glm::vec4;

ScreenMapper::ScreenMapper():
	PipeStage("Viewport Transform", DrawEngine::getDrawEngine())
{
}

void ScreenMapper::setViewPort(int x, int y, int w, int h)
{
	mViewport.x = x;
	mViewport.y = y;
	mViewport.w = w;
	mViewport.h = h;
}

void ScreenMapper::getViewPort(int &x, int &y, int &w, int &h)
{
	x = mViewport.x;
	y = mViewport.y;
	w = mViewport.w;
	h = mViewport.h;
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
			pos.x = mViewport.x + (pos.x + 1.0f) * mViewport.w / 2.0;
			pos.y = mViewport.y + (pos.y + 1.0f) * mViewport.h / 2.0;
		}
	}
#endif
}

void ScreenMapper::finalize()
{
}

NS_CLOSE_GLSP_OGL()
