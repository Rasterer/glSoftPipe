#include "PipeStage.h"
#include "DrawEngine.h"

namespace glsp {

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

} // namespace glsp;
