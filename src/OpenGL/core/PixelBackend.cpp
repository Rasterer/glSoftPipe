#include "PixelBackend.h"

#include <glm/glm.hpp>
#include "DataFlow.h"
#include "GLContext.h"
#include "utils.h"

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
	const int &index0 = fsio.y * g_GC->mRT.width + fsio.x;
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
#if 0
	Fsio *pFsio = static_cast<Fsio *>(data);

	onBlending(*pFsio);
#endif

	Fsiosimd *pFsio = static_cast<Fsiosimd *>(data);

	onBlendingSIMD(*pFsio);
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

static inline void ColorBlending(int32_t *colorBuffer, int index, __m128 &vInputColor)
{
	__m128i vSrcColori;
	__m128  vSrcColor;
	__m128i vDestColori;
	__m128  vDestColor;

	static const __m128i mask = _mm_set_epi8(0x80, 0x80, 0x80, 0x80,
								0x80, 0x80, 0x80, 0x80,
								0x80, 0x80, 0x80, 0x80,
								0x0C, 0x08, 0x04, 0x00);

	vDestColori = _mm_cvtsi32_si128(colorBuffer[index]);
	vDestColori = _mm_unpacklo_epi8 (vDestColori, _mm_setzero_si128());
	vDestColori = _mm_unpacklo_epi16(vDestColori, _mm_setzero_si128());
	vDestColor  = _mm_cvtepi32_ps(vDestColori);

	float src_alpha = _mm_cvtss_f32(_mm_shuffle_ps(vInputColor, vInputColor, _MM_SHUFFLE(0, 0, 0, 3)));
	vSrcColor  = _mm_mul_ps(vInputColor, _mm_set_ps1(256.0f));
	vSrcColor  = _simd_lerp_ps(vDestColor, vSrcColor, src_alpha);
	vSrcColori = _simd_clamp_epi32(_mm_cvtps_epi32(vSrcColor), _mm_setzero_si128(), _mm_set1_epi32(255));
	vSrcColori = _mm_shuffle_epi8(vSrcColori, mask);
	colorBuffer[index] = _mm_cvtsi128_si32(vSrcColori);;
}

void Blender::onBlendingSIMD(Fsiosimd &fsio)
{
	int32_t *colorBuffer = (int32_t *)g_GC->mRT.pColorBuffer;
	int index;

	// left-bottom pixel
	if (fsio.mCoverageMask & 1)
	{
		index = fsio.y * g_GC->mRT.width + fsio.x;
		ColorBlending(colorBuffer, index, fsio.mOutRegs[0]);
	}

	// right-bottom pixel
	if (fsio.mCoverageMask & 2)
	{
		index = fsio.y * g_GC->mRT.width + fsio.x + 1;
		ColorBlending(colorBuffer, index, fsio.mOutRegs[1]);
	}

	// left-top pixel
	if (fsio.mCoverageMask & 4)
	{
		index = (fsio.y + 1) * g_GC->mRT.width + fsio.x;
		ColorBlending(colorBuffer, index, fsio.mOutRegs[2]);
	}

	// right-top pixel
	if (fsio.mCoverageMask & 8)
	{
		index = (fsio.y + 1) * g_GC->mRT.width + fsio.x + 1;
		ColorBlending(colorBuffer, index, fsio.mOutRegs[3]);
	}
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
	onFBWritingSIMD(*pFsio);
#endif
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

	static __m128i mask = _mm_set_epi8(0x80, 0x80, 0x80, 0x80,
								0x80, 0x80, 0x80, 0x80,
								0x80, 0x80, 0x80, 0x80,
								0x0C, 0x08, 0x04, 0x00);
	__m128i tmp;

	// left-bottom pixel
	if (fsio.mCoverageMask & 1)
	{
		index = fsio.y * g_GC->mRT.width + fsio.x;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[0], _mm_set_ps1(256.0f)));
		tmp   = _simd_clamp_epi32(tmp, _mm_setzero_si128(), _mm_set1_epi32(255));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}

	// right-bottom pixel
	if (fsio.mCoverageMask & 2)
	{
		index = fsio.y * g_GC->mRT.width + fsio.x + 1;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[1], _mm_set_ps1(256.0f)));
		tmp   = _simd_clamp_epi32(tmp, _mm_setzero_si128(), _mm_set1_epi32(255));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}

	// left-top pixel
	if (fsio.mCoverageMask & 4)
	{
		index = (fsio.y + 1) * g_GC->mRT.width + fsio.x;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[2], _mm_set_ps1(256.0f)));
		tmp   = _simd_clamp_epi32(tmp, _mm_setzero_si128(), _mm_set1_epi32(255));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}

	// right-top pixel
	if (fsio.mCoverageMask & 8)
	{
		index = (fsio.y + 1) * g_GC->mRT.width + fsio.x + 1;
		tmp   = _mm_cvtps_epi32(_mm_mul_ps(fsio.mOutRegs[3], _mm_set_ps1(256.0f)));
		tmp   = _simd_clamp_epi32(tmp, _mm_setzero_si128(), _mm_set1_epi32(255));
		tmp   = _mm_shuffle_epi8(tmp, mask);
		colorBuffer[index] = _mm_cvtsi128_si32(tmp);;
	}
}

} // namespace glsp
