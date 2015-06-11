#include "PixelBackend.h"


NS_OPEN_GLSP_OGL()

ZTester::ZTester():
	PipeStage("Depth Test", DrawEngine::getDrawEngine())
{
}

void ZTester::emit(void *data)
{
	Fsio *pFsio = static_cast<Fsio *>(data);

	bool success = onDepthTesting(*pFsio);

	if(!success)
		return;

	getNextStage()->emit(pFsio);
}

bool ZTester::onDepthTesting(Fsio &fsio)
{
	int index = (gRT->height - fsio.y - 1) * gRT->width + fsio.x;

	return (fsio.z < gRT->pDepthBuffer[index]);
}

NS_CLOSE_GLSP_OGL()