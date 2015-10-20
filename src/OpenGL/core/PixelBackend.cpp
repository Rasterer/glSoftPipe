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

	if(success)
		// TODO: depth mask
		gRT->pDepthBuffer[pFsio->mIndex] = pFsio->z;
	else
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
	const int &index = fsio.mIndex;
	glm::vec4 &src = fsio.out.fragcolor();
	uint8_t *dst = (uint8_t *)gRT->pColorBuffer;

	src.r = (uint8_t)((src.r * src.a + dst[4*index+2] * (1 - src.a) / 256.0f));
	src.g = (uint8_t)((src.g * src.a + dst[4*index+1] * (1 - src.a) / 256.0f));
	src.b = (uint8_t)((src.b * src.a + dst[4*index+0] * (1 - src.a) / 256.0f));
	src.r = (uint8_t)((src.r * src.a + dst[4*index+3] * (1 - src.a) / 256.0f));
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

	// FIXME: How could we know this is a BGRA format buffer?
	colorBuffer[4 * index+2] = (uint8_t)(fsio.out.fragcolor().x * 256);
	colorBuffer[4 * index+1] = (uint8_t)(fsio.out.fragcolor().y * 256);
	colorBuffer[4 * index+0] = (uint8_t)(fsio.out.fragcolor().z * 256);
	colorBuffer[4 * index+3] = (uint8_t)(fsio.out.fragcolor().w * 256);
}

NS_CLOSE_GLSP_OGL()
