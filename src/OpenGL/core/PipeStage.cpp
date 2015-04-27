#include "PipeStage.h"
#include "DrawEngine.h"

NS_OPEN_GLSP_OGL()

using std::string;

PipeStage::PipeStage(const string &name, const DrawEngine& de):
	mDrawEngine(de),
	mName(name)
{
}

NS_CLOSE_GLSP_OGL();
