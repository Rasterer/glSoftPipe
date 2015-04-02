#include "PipeStage.h"
#include "DrawEngine.h"

PipeStage::PipeStage(const string &name, DrawEngine *de):
	mDrawEngine(de),
	mName(name)
{
}

DrawContext * PipeStage::getDrawCtx()
{
	return mDrawEngine->getDrawContext();
}
