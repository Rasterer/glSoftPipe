#include "PixelBackend.h"

#include <glm/glm.hpp>
#include "DataFlow.h"
#include "GLContext.h"


NS_OPEN_GLSP_OGL()

OwnershipTester::OwnershipTester():
	PipeStage("Ownership Test", DrawEngine::getDrawEngine())
{
}

ScissorTester::ScissorTester():
	PipeStage("Scissor Test", DrawEngine::getDrawEngine())
{
}

StencilTester::StencilTester():
	PipeStage("Stencil Test", DrawEngine::getDrawEngine())
{
}

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

bool ZTester::onDepthTesting(const Fsio &fsio)
{
	return (fsio.z < gRT->pDepthBuffer[fsio.mIndex]);
}

Blender::Blender():
	PipeStage("Blend", DrawEngine::getDrawEngine())
{
}

void Blender::emit(void *data)
{
	Fsio *pFsio = static_cast<Fsio *>(data);

	onBlending(*pFsio);

	getNextStage()->emit(pFsio);
}

// TODO: impl
void Blender::onBlending(Fsio &fsio)
{
	return;
}

Dither::Dither():
	PipeStage("Dithering", DrawEngine::getDrawEngine())
{
}

FBWriter::FBWriter():
	PipeStage("Writing FB", DrawEngine::getDrawEngine())
{
}

void FBWriter::emit(void *data)
{
	Fsio *pFsio = static_cast<Fsio *>(data);

	onFBWriting(*pFsio);
}

void FBWriter::onFBWriting(const Fsio &fsio)
{
	const int &index = fsio.mIndex;
	uint8_t *colorBuffer = (uint8_t *)gRT->pColorBuffer;

	colorBuffer[4 * index+2] = (uint8_t)(fsio.out.fragcolor().x * 256);
	colorBuffer[4 * index+1] = (uint8_t)(fsio.out.fragcolor().y * 256);
	colorBuffer[4 * index+0] = (uint8_t)(fsio.out.fragcolor().z * 256);
	//colorBuffer[4 * index+3] = (unsigned char)(fsio.out.fragcolor().w * 256);
	colorBuffer[4 * index+3] = 255;
}

NS_CLOSE_GLSP_OGL()