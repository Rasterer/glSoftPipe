#include "PipeStage.h"
#include "DrawEngine.h"

using std::string;

PipeStage::PipeStage(const string &name, const DrawEngine& de):
	mDrawEngine(de),
	mName(name)
{
}
