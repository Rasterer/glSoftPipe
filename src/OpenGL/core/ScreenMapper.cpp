#include "ScreenMapper.h"
#include "DataFlow.h"
#include "DrawEngine.h"
#include "GLContext.h"


NS_OPEN_GLSP_OGL()

using glm::vec4;

ScreenMapper::ScreenMapper():
	PipeStage("Viewport Transform", DrawEngine::getDrawEngine())
{
}

//void ScreenMapper::setViewPort(int x, int y, int w, int h)
//{
	//mViewport.x = x;
	//mViewport.y = y;
	//mViewport.w = w;
	//mViewport.h = h;
//}

//void ScreenMapper::getViewPort(int &x, int &y, int &w, int &h)
//{
	//x = mViewport.x;
	//y = mViewport.y;
	//w = mViewport.w;
	//h = mViewport.h;
//}

void ScreenMapper::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	viewportTransform(bat);

	getNextStage()->emit(bat);
}

void ScreenMapper::viewportTransform(Batch *bat)
{
	__GET_CONTEXT();

	int xCenter = gc->mState.mViewport.xCenter;
	int yCenter = gc->mState.mViewport.yCenter;
	int xScale  = gc->mState.mViewport.xScale;
	int yScale  = gc->mState.mViewport.yScale;

//#if PRIMITIVE_OWNS_VERTICES
#if 1
	PrimBatch &in = bat->mPrims;

	for(auto it = in.begin(); it != in.end(); ++it)
	{
		for(size_t i = 0; i < 3; ++i)
		{
			vec4 &pos = it->mVert[i].position();
			pos.x = xCenter + pos.x * xScale;
			pos.y = yCenter + pos.y * yScale;
		}
	}
#endif
}

void ScreenMapper::finalize()
{
}

NS_CLOSE_GLSP_OGL()
