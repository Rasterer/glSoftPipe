#include "Rasterizer.h"

#include "DrawEngine.h"


NS_OPEN_GLSP_OGL()

using glm::vec3;
using glm::vec4;

Rasterizer::Rasterizer():
	PipeStage("Rasterizing", DrawEngine::getDrawEngine())
{
}


void Rasterizer::emit(void *data)
{
	DrawContext *dc = static_cast<DrawContext *>(data);
	onRasterizing(dc);

	finalize();
}


void Rasterizer::finalize()
{
}

Interpolater::Interpolater():
	PipeStage("Interpolating", DrawEngine::getDrawEngine())
{
}

void Interpolater::emit(void *data)
{
	getNextStage()->emit(data);
}


void Interpolater::finalize()
{
}

NS_CLOSE_GLSP_OGL()
