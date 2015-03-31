#include "PipeStage.h"

DrawContext * PipeStage::getDrawCtx()
{
	return mDE->getDC();
}
