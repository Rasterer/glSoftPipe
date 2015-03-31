#include "PipeStage.h"

PipeStage::PipeStage(string &name)
	mName(name)
{
}

DrawContext * PipeStage::getDrawCtx()
{
	return mDE->getDC();
}
