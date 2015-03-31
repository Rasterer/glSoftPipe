#include "DrawEngine.h"

DrawEngine::DrawEngine()
{
}

DrawEngine::init()
{
}

void DrawEngine::validateState(GLContext *gc)
{
}

void DrawEngine::emit(DrawContext *dc, GLContext *gc)
{
	Batch *bat = mAsbl.assemble(dc, gc);

	mPP->

}
