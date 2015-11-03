#include "PipeStage.h"
#include "DrawEngine.h"

NS_OPEN_GLSP_OGL()

using std::string;

PipeStage::PipeStage(const string &name, const DrawEngine& de):
	mDrawEngine(de),
	mName(name)
{
}

PipeStageChain::PipeStageChain(const std::string &name, const DrawEngine& de):
	PipeStage(name, de),
	mFirstChild(nullptr),
	mLastChild(nullptr)
{
}

NS_CLOSE_GLSP_OGL();
