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

	Primlist &in = bat->mPrims;

	for(auto it = in.begin(); it != in.end(); ++it)
	{
		for(size_t i = 0; i < 3; ++i)
		{
			vec4 &pos = it->mVert[i].position();
			pos.x = xCenter + pos.x * xScale;
			pos.y = yCenter + pos.y * yScale;
		}
	}
}

void ScreenMapper::finalize()
{
}

NS_CLOSE_GLSP_OGL()
