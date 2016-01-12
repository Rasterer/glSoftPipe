#include "PixelBackend.h"

#include <glm/glm.hpp>
#include "DataFlow.h"
#include "GLContext.h"


namespace glsp {

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
#if 0
	Fsio &fsio = *static_cast<Fsio *>(data);
	onDepthTesting(fsio);
#else
	Fsiosimd &fsio = *static_cast<Fsiosimd *>(data);
	onDepthTestingSIMD(fsio);
#endif

	getNextStage()->emit(data);
}

bool ZTester::onDepthTesting(const Fsio &fsio)
{
	if (fsio.z < g_GC->mRT.pDepthBuffer[fsio.mIndex])
		// TODO: depth mask
		g_GC->mRT.pDepthBuffer[fsio.mIndex] = fsio.z;

	return true;
}

bool ZTester::onDepthTestingSIMD(Fsiosimd &fsio)
{
	const int &index0 = (g_GC->mRT.height - (fsio.y) - 1) * g_GC->mRT.width + fsio.x;
	const int &index1 = index0 + 1;
	const int &index2 = index0 - g_GC->mRT.width;
	const int &index3 = index2 + 1;

	__m128 vDepth = _mm_set_ps(
			g_GC->mRT.pDepthBuffer[index3],
			g_GC->mRT.pDepthBuffer[index2],
			g_GC->mRT.pDepthBuffer[index1],
			g_GC->mRT.pDepthBuffer[index0]);

	int result = _mm_movemask_ps(_mm_cmp_ps(fsio.mInRegs[3], vDepth, _CMP_LT_OS));
	result &= fsio.mCoverageMask;
	fsio.mCoverageMask = result;

	ALIGN(16) float z[4];
	if (result)
	{
		_mm_store_ps(z, fsio.mInRegs[3]);
	}
	if (result & 1)
	{
		g_GC->mRT.pDepthBuffer[index0] = z[0];
	}
	if (result & 2)
	{
		g_GC->mRT.pDepthBuffer[index0] = z[1];
	}
	if (result & 4)
	{
		g_GC->mRT.pDepthBuffer[index0] = z[2];
	}
	if (result & 8)
	{
		g_GC->mRT.pDepthBuffer[index0] = z[3];
	}

	return true;
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
	uint8_t *dst = (uint8_t *)g_GC->mRT.pColorBuffer;

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
#if 0
	Fsio *pFsio = static_cast<Fsio *>(data);
	onFBWriting(*pFsio);
#else
	Fsiosimd *pFsio = static_cast<Fsiosimd *>(data);
#endif
	onFBWritingSIMD(*pFsio);
}

void FBWriter::onFBWriting(const Fsio &fsio)
{
	const int &index = fsio.mIndex;
	uint8_t *colorBuffer = (uint8_t *)g_GC->mRT.pColorBuffer;

	colorBuffer[4 * index+0] = (uint8_t)(fsio.out.fragcolor().x * 256);
	colorBuffer[4 * index+1] = (uint8_t)(fsio.out.fragcolor().y * 256);
	colorBuffer[4 * index+2] = (uint8_t)(fsio.out.fragcolor().z * 256);
	colorBuffer[4 * index+3] = (uint8_t)(fsio.out.fragcolor().w * 256);
}

void FBWriter::onFBWritingSIMD(const Fsiosimd &fsio)
{
	int32_t *colorBuffer = (int32_t *)g_GC->mRT.pColorBuffer;
	int index;

	__m128i tmp;
	__m128i mask = _mm_set_epi8(0x80, 0x80, 0x80, 0x80,
								0x80, 0x80, 0x80, 0x80,
								0x80, 0x80, 0x80, 0x80,
								0x0C, 0x08, 0x04, 0x00);
	// left-bottom pixel
	if (fsio.mCoverageMask & 1)
	{
		index = (g_GC->mRT.height - (fsio.y) - 1) * g_GC->mRT.width + fsio.x;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[0], _mm_set_ps1(256.0f)));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}

	// right-bottom pixel
	if (fsio.mCoverageMask & 2)
	{
		index = (g_GC->mRT.height - (fsio.y) - 1) * g_GC->mRT.width + fsio.x + 1;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[1], _mm_set_ps1(256.0f)));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}

	// left-top pixel
	if (fsio.mCoverageMask & 4)
	{
		index = (g_GC->mRT.height - (fsio.y + 1) - 1) * g_GC->mRT.width + fsio.x;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[2], _mm_set_ps1(256.0f)));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}

	// right-top pixel
	if (fsio.mCoverageMask & 8)
	{
		index = (g_GC->mRT.height - (fsio.y + 1) - 1) * g_GC->mRT.width + fsio.x + 1;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[3], _mm_set_ps1(256.0f)));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}
}

} // namespace glsp
