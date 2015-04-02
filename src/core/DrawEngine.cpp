#include "DrawEngine.h"

DrawEngine::DrawEngine()
{
}

// TODO: connect all the pipeline stages
void DrawEngine::init()
{
}

void DrawEngine::validateState(GLContext *gc)
{
}

void DrawEngine::emit(DrawContext *dc, GLContext *gc)
{
	getFirstStage()->emit(dc);
}

DrawEngine DrawEngine::DE;
