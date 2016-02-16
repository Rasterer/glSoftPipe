#include "Texture.h"
#include "DrawEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "GLContext.h"
#include "TBDR.h"
#include "compiler.h"


using glm::vec2;
using glm::vec4;

namespace glsp {

GLAPI void APIENTRY glGenTextures (GLsizei n, GLuint *textures)
{
	__GET_CONTEXT();

	gc->mTM.GenTextures(gc, n, textures);
}

GLAPI void APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures)
{
	__GET_CONTEXT();

	gc->mTM.DeleteTextures(gc, n, textures);
}

GLAPI void APIENTRY glBindTexture (GLenum target, GLuint texture)
{
	__GET_CONTEXT();

	gc->mTM.BindTexture(gc, target, texture);
}

GLAPI void APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
	__GET_CONTEXT();

	gc->mTM.TexImage2D(gc, target, level, internalformat, width, height, border, format, type, pixels);
}

GLAPI void APIENTRY glActiveTexture (GLenum texture)
{
	__GET_CONTEXT();

	gc->mTM.ActiveTexture(gc, texture);
}

GLAPI void APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	__GET_CONTEXT();

	gc->mTM.TexParameteri(gc, target, pname, param);
}

GLAPI void APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
	__GET_CONTEXT();

	gc->mTM.TexParameterfv(gc, target, pname, params);
}

GLAPI GLboolean APIENTRY glIsTexture (GLuint texture)
{
	__GET_CONTEXT();
	return gc->mTM.IsTexture(gc, texture);
}

namespace {

typedef void (*PFNCopyTexels)(const void *pSrc, TextureMipmap *pMipmap);


void CopyTexelIntactly(const uint8_t *pSrc, TextureMipmap *pMipmap)
{
	memcpy(pMipmap->mMem.addr,
		   pSrc,
		   pMipmap->mHeight * pMipmap->mWidth * pMipmap->mpTex->getBytesPerTexel());
}

void CopyTexelR8G8B8ToR5G6B5(const uint8_t *pSrc, TextureMipmap *pMipmap)
{
	uint32_t s = pMipmap->mWidth * pMipmap->mHeight;
	uint16_t *pDst = static_cast<uint16_t *>(pMipmap->mMem.addr);

	for(uint32_t i = 0; i < s; i++, pDst++)
	{
		*pDst  = ((*pSrc++) >> 3) << 0;
		*pDst |= ((*pSrc++) >> 2) << 5;
		*pDst |= ((*pSrc++) >> 3) << 11;
	}
}


void CopyTexelR8G8B8A8ToRGBAF(const uint8_t *pSrc, TextureMipmap *pMipmap)
{
	uint32_t s = pMipmap->mWidth * pMipmap->mHeight * 4;
	float *pDst = static_cast<float *>(pMipmap->mMem.addr);

	for(uint32_t i = 0; i < s; i++)
	{
		*pDst++ = *pSrc++ / 256.0f;
	}
}

void CopyTexelR8G8B8ToRGBAF(const uint8_t *pSrc, TextureMipmap *pMipmap)
{
	uint32_t s = pMipmap->mWidth * pMipmap->mHeight;
	float *pDst = static_cast<float *>(pMipmap->mMem.addr);

	for(uint32_t i = 0; i < s; i++)
	{
		*pDst++ = *pSrc++ / 256.0f;
		*pDst++ = *pSrc++ / 256.0f;
		*pDst++ = *pSrc++ / 256.0f;
		*pDst++ = 1.0f;		// fill 1.0f in the alpha component
	}
}

void CopyTexelR5G6B5ToRGBAF(const uint8_t *pSrc, TextureMipmap *pMipmap)
{
	uint32_t s = pMipmap->mWidth * pMipmap->mHeight;
	float *pDst = static_cast<float *>(pMipmap->mMem.addr);
	uint16_t *src = (uint16_t *)pSrc;

	for(uint32_t i = 0; i < s; i++)
	{
		*pDst++ = (*src >> 11) / 32.0f;
		*pDst++ = ((*src >> 5) & 0x3F) / 64.0f;
		*pDst++ = (*src & 0x1F) / 32.0f;
		*pDst++ = 1.0f;		// fill 1.0f in the alpha component
	}
}

// TODO: add support for other formats
bool PickupCopyFunc(int internalformat, unsigned format, unsigned type, PFNCopyTexels *ppfnCopyTexels)
{
	GLSP_UNREFERENCED_PARAM(internalformat);

	switch (format)
	{
		case GL_RGBA:
		{
			switch (type)
			{
				case GL_UNSIGNED_BYTE:
				{
					*ppfnCopyTexels = (PFNCopyTexels)CopyTexelIntactly;
					return true;
				}
				default:
				{
					return false;
				}
			}
		}
		case GL_RGB:
		{
			switch (type)
			{
				case GL_UNSIGNED_BYTE:
				{
					*ppfnCopyTexels = (PFNCopyTexels)CopyTexelR8G8B8ToRGBAF;
					return true;
				}
				case GL_UNSIGNED_SHORT_5_6_5:
				{
					*ppfnCopyTexels = (PFNCopyTexels)CopyTexelR5G6B5ToRGBAF;
					return true;
				}
				default:
				{
					return false;
				}
			}
		}
		case GL_DEPTH_COMPONENT:
		{
			switch (type)
			{
				case GL_FLOAT:
				{
					*ppfnCopyTexels = (PFNCopyTexels)CopyTexelIntactly;
					return true;
				}
				default:
				{
					return false;
				}
			}
		}
		default:
		{
			return false;
		}
	}
}

uint32_t TargetToMipmapLevels(unsigned target)
{
	switch(target)
	{
		case GL_TEXTURE_2D:
			return MAX_TEXTURE_MIPMAP_LEVELS;
		default:
			return 1;
	}
}

TextureTarget TargetToIndex(unsigned target)
{
	switch(target)
	{
		case GL_TEXTURE_1D:
			return TEXTURE_TARGET_1D;
		case GL_TEXTURE_2D:
			return TEXTURE_TARGET_2D;
		case GL_TEXTURE_3D:
			return TEXTURE_TARGET_2D;
		case GL_TEXTURE_1D_ARRAY:
			return TEXTURE_TARGET_1D_ARRAY;
		case GL_TEXTURE_2D_ARRAY:
			return TEXTURE_TARGET_2D_ARRAY;
		case GL_TEXTURE_RECTANGLE:
			return TEXTURE_TARGET_RECTANGLE;
		case GL_TEXTURE_BUFFER:
			return TEXTURE_TARGET_BUFFER;
		case GL_TEXTURE_CUBE_MAP:
			return TEXTURE_TARGET_CUBE_MAP;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			return TEXTURE_TARGET_CUBE_MAP_ARRAY;
		case GL_TEXTURE_2D_MULTISAMPLE:
			return TEXTURE_TARGET_2D_MULTISAMPLE;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			return TEXTURE_TARGET_2D_MULTISAMPLE_ARRAY;
		default:
			return TEXTURE_TARGET_UNBOUND;
	}
}

#if 0
inline void BiLinearInterpolate(float scaleX, float scaleY, const vec4 &lb, const vec4 &lt, const vec4 &rb, const vec4 &rt, vec4 &res)
{
	res = lb * ((1.0f - scaleX) * (1.0f - scaleY)) +
		  lt * ((1.0f - scaleX) * scaleY) +
		  rb * (scaleX * (1.0f - scaleY)) +
		  rt * (scaleX * scaleY);
}

void Sample2DNearestLevel(const Texture *pTex, unsigned int level, const vec2 &coord, vec4 &res)
{
	const TextureMipmap *pMipmap = pTex->getMipmap(0, level);

	int i = pTex->m_pfnWrapS(static_cast<int>(std::floor(coord.x * pMipmap->mWidth)), pMipmap->mWidth);
	int j = pTex->m_pfnWrapT(static_cast<int>(std::floor(coord.y * pMipmap->mHeight)), pMipmap->mHeight);

	if(i < 0 || i >= (int)pMipmap->mWidth ||
	   j < 0 || j >= (int)pMipmap->mHeight)
	{
		res = pTex->getSamplerObject().mBorderColor;
		return;
	}

	vec4 *pAddr = static_cast<vec4 *>(pMipmap->mMem.addr);

	res = pAddr[j * pMipmap->mWidth + i];
}

void Sample2DLinearLevel(const Texture *pTex, unsigned int level, const vec2 &coord, vec4 &res)
{
	const TextureMipmap *pMipmap = pTex->getMipmap(0, level);

	float TexSpaceX = coord.x * pMipmap->mWidth  - 0.5f;
	float TexSpaceY = coord.y * pMipmap->mHeight - 0.5f;

	int i0 = pTex->m_pfnWrapS(static_cast<int>(floor(TexSpaceX)), pMipmap->mWidth);
	int j0 = pTex->m_pfnWrapT(static_cast<int>(floor(TexSpaceY)), pMipmap->mHeight);

	int i1 = pTex->m_pfnWrapS(static_cast<int>(floor(TexSpaceX)) + 1, pMipmap->mWidth);
	int j1 = pTex->m_pfnWrapT(static_cast<int>(floor(TexSpaceY)) + 1, pMipmap->mHeight);

	if(i0 < 0 || i0 >= (int)pMipmap->mWidth  ||
	   j0 < 0 || j0 >= (int)pMipmap->mHeight ||
	   i1 < 0 || i1 >= (int)pMipmap->mWidth  ||
	   j1 < 0 || j1 >= (int)pMipmap->mHeight)
	{
		res = pTex->getSamplerObject().mBorderColor;
		return;
	}

	vec4 *pAddr = static_cast<vec4 *>(pMipmap->mMem.addr);

	vec4 lb = pAddr[j0 * pMipmap->mWidth + i0];
	vec4 lt = pAddr[j1 * pMipmap->mWidth + i0];
	vec4 rb = pAddr[j0 * pMipmap->mWidth + i1];
	vec4 rt = pAddr[j1 * pMipmap->mWidth + i1];

	float intpart;
	float scaleX = std::modf(TexSpaceX, &intpart);
	float scaleY = std::modf(TexSpaceY, &intpart);

	BiLinearInterpolate(scaleX, scaleY, lb, lt, rb, rt, res);
}

void Sample2DNearestNonmipmap(const Texture *pTex, float, const vec2 &coord, vec4 &res)
{
	Sample2DNearestLevel(pTex, 0, coord, res);
}

void Sample2DLinearNonmipmap(const Texture *pTex, float, const vec2 &coord, vec4 &res)
{
	Sample2DLinearLevel(pTex, 0, coord, res);
}

void Sample2DNearestMipmapNearest(const Texture *pTex, float lambda, const vec2 &coord, vec4 &res)
{
	unsigned int lvl;

	if(lambda <= 0.5f)
		lvl = 0;
	else
		lvl = (unsigned)std::ceil(lambda + 0.5f) - 1;

	if(lvl > pTex->getAvailableMipmaps())
		lvl = pTex->getAvailableMipmaps();

	Sample2DNearestLevel(pTex, lvl, coord, res);
}

void Sample2DLinearMipmapNearest(const Texture *pTex, float lambda, const vec2 &coord, vec4 &res)
{
	unsigned int lvl;

	if(lambda <= 0.5f)
		lvl = 0;
	else
		lvl = (unsigned)std::ceil(lambda + 0.5f) - 1;

	if(lvl > pTex->getAvailableMipmaps())
		lvl = pTex->getAvailableMipmaps();

	Sample2DLinearLevel(pTex, lvl, coord, res);
}

void Sample2DNearestMipmapLinear(const Texture *pTex, float lambda, const vec2 &coord, vec4 &res)
{
	uint32_t lvl1, lvl2;

	lvl1 = (uint32_t)std::floor(lambda);

	if(lvl1 >= pTex->getAvailableMipmaps())
	{
		lvl1 = pTex->getAvailableMipmaps();
		lvl2 = lvl1;

		Sample2DNearestLevel(pTex, lvl1, coord, res);
	}
	else
	{
		lvl2 = lvl1 + 1;

		vec4 texel1, texel2;

		Sample2DNearestLevel(pTex, lvl1, coord, texel1);
		Sample2DNearestLevel(pTex, lvl2, coord, texel2);

		float intpart;
		float frac = std::modf(lambda, &intpart);

		res = texel1 * (1.0f - frac) + texel2 * frac;
	}
}

void Sample2DLinearMipmapLinear(const Texture *pTex, float lambda, const vec2 &coord, vec4 &res)
{
	uint32_t lvl1, lvl2;

	lvl1 = (uint32_t)std::floor(lambda);

	if(lvl1 >= pTex->getAvailableMipmaps())
	{
		lvl1 = pTex->getAvailableMipmaps();
		lvl2 = lvl1;

		Sample2DLinearLevel(pTex, lvl1, coord, res);
	}
	else
	{
		lvl2 = lvl1 + 1;

		vec4 texel1, texel2;

		Sample2DLinearLevel(pTex, lvl1, coord, texel1);
		Sample2DLinearLevel(pTex, lvl2, coord, texel2);

		float intpart;
		float frac = std::modf(lambda, &intpart);

		res = texel1 * (1.0f - frac) + texel2 * frac;
	}
}

void Sample2DNearest(const Shader *, const Texture *pTex, const vec2 &coord, vec4 &res)
{
	Sample2DNearestLevel(pTex, 0, coord, res);
}

void Sample2DLinear(const Shader *, const Texture *pTex, const vec2 &coord, vec4 &res)
{
	Sample2DLinearLevel(pTex, 0, coord, res);
}

void Sample2DLambda(const Shader *pShader, const Texture *pTex, const vec2 &coord, vec4 &res)
{
	TextureMipmap *pMipmap = pTex->getBaseMipmap(0);
	float lambda = ComputeLambda(pShader, coord, pMipmap->mWidth, pMipmap->mHeight);

	if(lambda <= pTex->getMagMinThresh())
	{
		pTex->m_pfnTexture2DMag(pTex, 0, coord, res);
	}
	else
	{
		pTex->m_pfnTexture2DMin(pTex, lambda, coord, res);
	}
}

inline int mirror(int val)
{
	return (val >= 0) ? val: (-1 - val);
}

int WrapClampToEdge(int coord, int size)
{
	return clamp(coord, 0, size - 1);
}

int WrapClampToBorder(int coord, int size)
{
	return clamp(coord, -1, size);
}

int WrapRepeat(int coord, int size)
{
	return (coord >= 0) ? (coord % size): ((size - (-coord) % size) % size);
}

int WrapMirroredRepeat(int coord, int size)
{
	return (size - 1) - mirror(coord % (2 * size) - size);
}

int WrapMirrorClampToEdge(int coord, int size)
{
	return clamp(mirror(coord), 0, size - 1);
}

__m128i WrapClampToEdgeSIMD(__m128i &coord, int size)
{
	return _mm_min_epi32(_mm_max_epi32(coord, _mm_setzero_si128()), _mm_set1_epi32(size - 1));
}

__m128i WrapClampToBorderSIMD(__m128i &coord, int size)
{
	return _mm_min_epi32(_mm_max_epi32(coord, _mm_set1_epi32(-1)), _mm_set1_epi32(size));
}

__m128i WrapRepeatSIMD(__m128i &coord, int size)
{
	// FIXME
	return (coord >= 0) ? (coord % size): ((size - (-coord) % size) % size);
}

__m128i WrapMirroredRepeatSIMD(__m128i &coord, int size)
{
	// FIXME
	return _mm_min_epi32(_mm_max_epi32(coord, _mm_set1_epi32(-1)), _mm_set1_epi32(size));
}

__m128i WrapMirrorClampToEdgeSIMD(__m128i &coord, int size)
{
	// FIXME
	return _mm_min_epi32(_mm_max_epi32(coord, _mm_set1_epi32(-1)), _mm_set1_epi32(size));
}
#endif

} /* namespace */


SamplerObject::SamplerObject():
	mBorderColor(0.0f, 0.0f, 0.0f, 0.0f),
	eWrapS(GL_REPEAT),
	eWrapT(GL_REPEAT),
	eWrapR(GL_REPEAT),
	eMagFilter(GL_LINEAR),
	eMinFilter(GL_NEAREST_MIPMAP_LINEAR)
{
}

TextureMipmap::TextureMipmap():
	mResident(false),
	mWidth(0),
	mHeight(0),
	mRefCount(1)
{
	mMem.size = 0;
	mMem.addr = NULL;
}

TextureMipmap::~TextureMipmap()
{
	if(mResident)
		free(mMem.addr);
}

Texture::Texture():
	mpMipmap(NULL),
	mAvailableMipmaps(0),
	mNumLayers(0), // FIXME: set layer num correctly
	mIsComplete(false)
{
}

Texture::Texture(unsigned target):
	mAvailableMipmaps(0),
	mTextureTarget(target),
	mNumLayers(1), // FIXME: set layer num correctly
	mIsComplete(false)
{
	mSO.setName(0);

	mNumMipmaps = TargetToMipmapLevels(target);

	mpMipmap = new TextureMipmap[mNumMipmaps];

	for(uint32_t i = 0; i < mNumMipmaps; i++)
	{
		mpMipmap[i].mLevel = i;
		mpMipmap[i].mpTex  = this;
	}
}

Texture& Texture::operator=(const Texture &rhs)
{
	memcpy(this, &rhs, sizeof(Texture));

	for(uint32_t i = 0; i < mNumMipmaps; ++i)
	{
		mpMipmap[i].mRefCount++;
	}

	return *this;
}

Texture::~Texture()
{
	if(mpMipmap)
	{
		for(uint32_t i = 0; i < mNumMipmaps; ++i)
		{
			mpMipmap[i].mRefCount--;
		}

		if(mpMipmap->mRefCount == 0)
			delete []mpMipmap;
	}
}

// TODO: add support for other targets
TextureMipmap* Texture::getMipmap(uint32_t layer, int32_t level) const
{
	switch(mTextureTarget)
	{
		case GL_TEXTURE_2D:
			assert(layer == 0);
			return &mpMipmap[level];
		default:
			return NULL;
	}
}

unsigned int GetBPPFromFormat(int internalformat, unsigned format)
{
	if (internalformat == GL_RGBA)
		return 4;

	if (internalformat == GL_DEPTH_COMPONENT)
		return 4;

	return 0;
}

// TODO: consider the consistency between mipmaps
void Texture::TexImage2D(int level, int internalformat,
				int width, int height, int border,
				unsigned format, unsigned type, const void *pixels)
{
	uint32_t layer = 0;

	if(border != 0)
		return;

	if(level >= MAX_TEXTURE_MIPMAP_LEVELS)
		return;

	if(level > (int)mAvailableMipmaps)
		mAvailableMipmaps = level;

	if(level != 0)
	{
		int req_w, req_h;
		TextureMipmap *pBaseLevel = getBaseMipmap(layer);

		if((req_w = pBaseLevel->mWidth >> level) == 0)
			req_w = 1;

		if((req_h = pBaseLevel->mHeight >> level) == 0)
			req_h = 1;

		if(req_w != width || req_h != height)
			return;
	}

	mFormat = internalformat;
	mBytesPerTexel = GetBPPFromFormat(internalformat, format);
	if (mBytesPerTexel == 0)
		return;

	PFNCopyTexels pfnCopyTexels;
	if (pixels)
	{
		if(!PickupCopyFunc(internalformat, format, type, &pfnCopyTexels))
		{
			return;
		}
	}
	else
	{
		pfnCopyTexels = nullptr;
	}

	uint32_t size = width * height * mBytesPerTexel;
	bool bNeedRealloc = true;

	TextureMipmap *pMipmap = getMipmap(layer, level);
	if(pMipmap->mResident)
	{
		if(size > pMipmap->mMem.size)
		{
			free(pMipmap->mMem.addr);
		}
		else
		{
			// Luckily, the current backup memory can be reused.
			bNeedRealloc = false;
		}
	}

	if(bNeedRealloc)
	{
		pMipmap->mMem.addr = malloc(size);
		pMipmap->mAllocationSize = size;
	}

	pMipmap->mSizeUsed = size;
	pMipmap->mWidth    = width;
	pMipmap->mHeight   = height;
	pMipmap->mResident = true;

	// If no initialization data, just alloc memory and leave it uninitialized.
	// TODO: add PBO support.
	if(pfnCopyTexels)
		pfnCopyTexels(pixels, pMipmap);
}

void Texture::TexParameteri(unsigned pname, int param)
{
	SamplerObject *pSO = &mSO;

	switch(pname)
	{
		case GL_TEXTURE_WRAP_S:
		{
			pSO->eWrapS = param;
			break;
		}
		case GL_TEXTURE_WRAP_T:
		{
			pSO->eWrapT = param;
			break;
		}
		case GL_TEXTURE_WRAP_R:
		{
			pSO->eWrapR = param;
			break;
		}
		case GL_TEXTURE_MAG_FILTER:
		{
			pSO->eMagFilter = param;
			break;
		}
		case GL_TEXTURE_MIN_FILTER:
		{
			pSO->eMinFilter = param;
			break;
		}
		default:
		{
			break;
		}
	}
}

void Texture::TexParameterfv(unsigned pname, const GLfloat *params)
{
	SamplerObject *pSO = &mSO;

	switch(pname)
	{
		case GL_TEXTURE_BORDER_COLOR:
		{
			pSO->mBorderColor.x = clamp(params[0], 0.0f, 1.0f);
			pSO->mBorderColor.y = clamp(params[1], 0.0f, 1.0f);
			pSO->mBorderColor.z = clamp(params[2], 0.0f, 1.0f);
			pSO->mBorderColor.w = clamp(params[3], 0.0f, 1.0f);

			break;
		}
		default:
		{
			break;
		}
	}
}

bool Texture::IsComplete()
{
	mIsComplete = false;

	if (mBytesPerTexel == 0)
		goto err;

	for(uint32_t i = 0; i <= mAvailableMipmaps; ++i)
	{
		if(!mpMipmap[i].mResident)
		{
			goto err;
		}
	}

	mIsComplete = true;
err:
	return mIsComplete;
}

static inline __m128i CoordinateWrapping(uint32_t size, const __m128i &vCoord, int wrap_mode)
{
	if (wrap_mode == GL_REPEAT)
	{
		__m128 vSize = _mm_set_ps1(size);
		__m128 vCoordf = _mm_cvtepi32_ps(vCoord);
		vCoordf = _mm_sub_ps(vCoordf, _mm_mul_ps(_mm_floor_ps(_mm_mul_ps(vCoordf, _mm_set_ps1(1.0f / size))), vSize));
		return _mm_cvtps_epi32(vCoordf);
	}
	else if (wrap_mode == GL_CLAMP_TO_EDGE)
	{
		return _mm_max_epi32(_mm_min_epi32(vCoord, _mm_set1_epi32(size - 1)), _mm_setzero_si128());
	}
	else
	{
		// TODO: other wrap modes
		return _mm_setzero_si128();
	}
}

inline void Texture::MipmapLerp(__m128 vRes1[4], __m128 vRes2[4], float frac, __m128 out[])
{
	switch (mFormat)
	{
		case GL_DEPTH_COMPONENT:
		{
			out[0] = _simd_lerp_ps(vRes1[0], vRes2[0], frac);
			break;
		}
		case GL_RGBA:
		default:
		{
			for (int i = 0; i < 4; ++i)
			{
				out[i] = _simd_lerp_ps(vRes1[i], vRes2[i], frac);
			}
			break;
		}
	}
}

float Texture::ComputeLambda(const __m128 &s, const __m128 &t)
{
	const TextureMipmap *pBaseMipmap = getMipmap(0, 0);
	const __m128 vU = _mm_mul_ps(s, _mm_set1_ps((float)pBaseMipmap->mWidth));
	const __m128 vV = _mm_mul_ps(t, _mm_set1_ps((float)pBaseMipmap->mHeight));

	ALIGN(16) float u[4];
	ALIGN(16) float v[4];

	_mm_store_ps(u, vU);
	_mm_store_ps(v, vV);

	float dudx = ((u[3] - u[2]) + (u[1] - u[0]));
	float dudy = ((u[3] - u[1]) + (u[2] - u[0]));
	float dvdx = ((v[3] - v[2]) + (v[1] - v[0]));
	float dvdy = ((v[3] - v[1]) + (v[2] - v[0]));

	return 0.5f * std::log2((std::max)(dudx * dudx + dvdx * dvdx, dudy * dudy + dvdy * dvdy)) - 1.0f;
}

void Texture::Texture2DSIMD(const __m128 &u, const __m128 &v, uint32_t out[])
{
	// TODO:
}

void Texture::Texture2DSIMD(const __m128 &s, const __m128 &t, __m128 out[])
{
	if (mSO.eMagFilter == mSO.eMinFilter)
	{
		// No need to calculate lambda.
		if (mSO.eMagFilter == GL_LINEAR)
		{
			Sample2DLinearLevelSIMD(0, s, t, out);
		}
		else
		{
			// GL_NEAREST
			Sample2DNearestLevelSIMD(0, s, t, out);
		}
	}
	else
	{
		// TODO:
		const float lambda = ComputeLambda(s, t);
		if(lambda <= mMagMinThresh)
		{
			// Mag filter
			if (mSO.eMagFilter == GL_LINEAR)
			{
				Sample2DLinearLevelSIMD(0, s, t, out);
			}
			else
			{
				// GL_NEAREST
				Sample2DNearestLevelSIMD(0, s, t, out);
			}
		}
		else
		{
			// Min filter
			unsigned lvl1 = 0;
			unsigned lvl2 = 0;
			float    frac = 0.0f;

			if (mAvailableMipmaps == 0 || mSO.eMinFilter == GL_LINEAR || GL_LINEAR == GL_NEAREST)
			{
				// only access the base level-of-detail.
				lvl1 = 0;
			}
			else if (mSO.eMinFilter == GL_LINEAR_MIPMAP_NEAREST || mSO.eMinFilter == GL_NEAREST_MIPMAP_NEAREST)
			{
				if(lambda <= 0.5f)
					lvl1 = 0;
				else
					lvl1 = (unsigned)std::ceil(lambda + 0.5f) - 1;

				if(lvl1 > mAvailableMipmaps)
					lvl1 = mAvailableMipmaps;
			}
			else if (mSO.eMinFilter == GL_NEAREST_MIPMAP_LINEAR || mSO.eMinFilter == GL_LINEAR_MIPMAP_LINEAR)
			{
				const float lvl1f = std::floor(lambda);
				lvl1 = static_cast<unsigned>(lvl1f);
				frac = lambda - lvl1f;

				if (lvl1 >= mAvailableMipmaps)
				{
					lvl1 = mAvailableMipmaps;
					lvl2 = mAvailableMipmaps;
				}
				else
				{
					lvl2 = lvl1 + 1;
				}
			}

			switch (mSO.eMinFilter)
			{
				case GL_LINEAR:
				case GL_LINEAR_MIPMAP_NEAREST:
				{
					Sample2DLinearLevelSIMD(lvl1, s, t, out);
					break;
				}
				case GL_NEAREST:
				case GL_NEAREST_MIPMAP_NEAREST:
				{
					Sample2DNearestLevelSIMD(lvl1, s, t, out);
					break;
				}
				case GL_NEAREST_MIPMAP_LINEAR:
				{
					if (lvl1 >= mAvailableMipmaps)
					{
						Sample2DNearestLevelSIMD(mAvailableMipmaps, s, t, out);
					}
					else
					{
						__m128 vRes1[4];
						__m128 vRes2[4];
						Sample2DNearestLevelSIMD(lvl1, s, t, vRes1);
						Sample2DNearestLevelSIMD(lvl2, s, t, vRes2);
						MipmapLerp(vRes1, vRes2, frac, out);
					}
					break;
				}
				case GL_LINEAR_MIPMAP_LINEAR:
				{
					if (lvl1 >= mAvailableMipmaps)
					{
						Sample2DLinearLevelSIMD(mAvailableMipmaps, s, t, out);
					}
					else
					{
						__m128 vRes1[4];
						__m128 vRes2[4];
						Sample2DLinearLevelSIMD(lvl1, s, t, vRes1);
						Sample2DLinearLevelSIMD(lvl2, s, t, vRes2);
						MipmapLerp(vRes1, vRes2, frac, out);
					}
					break;
				}
			}
		}
	}
}

void Texture::Sample2DNearestLevelSIMD(unsigned level, const __m128 &s, const __m128 &t, __m128 out[])
{
	const TextureMipmap *pMipmap = getMipmap(0, level);

	__m128 vTexSpaceS0 = _mm_mul_ps(s, _mm_set1_ps((float)pMipmap->mWidth));
	__m128 vTexSpaceT0 = _mm_mul_ps(t, _mm_set1_ps((float)pMipmap->mHeight));
	__m128 vTexS0Floor = _mm_floor_ps(vTexSpaceS0);
	__m128 vTexT0Floor = _mm_floor_ps(vTexSpaceT0);
	__m128i vS0 = _mm_cvtps_epi32(vTexS0Floor);
	__m128i vT0 = _mm_cvtps_epi32(vTexT0Floor);
	vS0 = CoordinateWrapping(pMipmap->mWidth,  vS0, mSO.eWrapS);
	vT0 = CoordinateWrapping(pMipmap->mWidth,  vT0, mSO.eWrapS);

	ALIGN(16) int addr[4];

	switch (mFormat)
	{
		case GL_RGBA:
		{
			uint32_t *pAddr = static_cast<uint32_t *>(pMipmap->mMem.addr);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS0));
			__m128i vLB = _mm_set_epi32(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			static __m128i vMask  = _mm_set1_epi32(0xFF);
			static __m128  vNorm = _mm_set_ps1(1.0f / 256.0f);
			for (int i = 0; i < 4; ++i)
			{
				out[i] = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(vLB, i << 3), vMask));
				out[i] = _mm_mul_ps(out[i], vNorm);
			}

			break;
		}
		case GL_DEPTH_COMPONENT:
		{
			float *pAddr = static_cast<float *>(pMipmap->mMem.addr);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS0));
			out[0] = _mm_set_ps(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			break;
		}
		default:
		{
			break;
		}
	}
}

void Texture::Sample2DLinearLevelSIMD(unsigned level, const __m128 &s, const __m128 &t, __m128 out[])
{
	const TextureMipmap *pMipmap = getMipmap(0, level);

	__m128 vTexSpaceS0 = MAWrapper(s, _mm_set1_ps((float)pMipmap->mWidth), _mm_set1_ps(-0.5f));
	__m128 vTexSpaceT0 = MAWrapper(t, _mm_set1_ps((float)pMipmap->mHeight), _mm_set1_ps(-0.5f));

	__m128 vTexS0Floor = _mm_floor_ps(vTexSpaceS0);
	__m128 vTexT0Floor = _mm_floor_ps(vTexSpaceT0);

	__m128i vTexSpaceS0i = _mm_cvtps_epi32(vTexS0Floor);
	__m128i vTexSpaceT0i = _mm_cvtps_epi32(vTexT0Floor);
	__m128i vTexSpaceS1i = _mm_add_epi32(vTexSpaceS0i, _mm_set1_epi32(1));
	__m128i vTexSpaceT1i = _mm_add_epi32(vTexSpaceT0i, _mm_set1_epi32(1));

	__m128i vS0 = CoordinateWrapping(pMipmap->mWidth,  vTexSpaceS0i, mSO.eWrapS);
	__m128i vS1 = CoordinateWrapping(pMipmap->mWidth,  vTexSpaceS1i, mSO.eWrapS);
	__m128i vT0 = CoordinateWrapping(pMipmap->mHeight, vTexSpaceT0i, mSO.eWrapT);
	__m128i vT1 = CoordinateWrapping(pMipmap->mHeight, vTexSpaceT1i, mSO.eWrapT);

	__m128 vScale_s      = _mm_sub_ps(vTexSpaceS0, vTexS0Floor);
	__m128 vScale_t      = _mm_sub_ps(vTexSpaceT0, vTexT0Floor);
	__m128 vScale_s_Comp = _mm_sub_ps(_mm_set_ps1(1.0f), vScale_s);
	__m128 vScale_t_Comp = _mm_sub_ps(_mm_set_ps1(1.0f), vScale_t);

	ALIGN(16) int addr[4];

	switch (mFormat)
	{
		case GL_RGBA:
		{
			uint32_t *pAddr = static_cast<uint32_t *>(pMipmap->mMem.addr);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS0));
			__m128i vLB = _mm_set_epi32(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS1));
			__m128i vRB = _mm_set_epi32(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT1, _mm_set1_epi32(pMipmap->mWidth), vS0));
			__m128i vLT = _mm_set_epi32(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT1, _mm_set1_epi32(pMipmap->mWidth), vS1));
			__m128i vRT = _mm_set_epi32(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			static __m128i vMask  = _mm_set1_epi32(0xFF);
			static __m128  vNorm = _mm_set_ps1(1.0f / 256.0f);
			for (int i = 0; i < 4; ++i)
			{
				__m128 vLBf = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(vLB, i << 3), vMask));
				__m128 vRBf = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(vRB, i << 3), vMask));
				__m128 vLTf = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(vLT, i << 3), vMask));
				__m128 vRTf = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(vRT, i << 3), vMask));

				out[i] = _mm_mul_ps(vRTf, _mm_mul_ps(vScale_s,     vScale_t     ));
				out[i] = MAWrapper(vRBf, _mm_mul_ps(vScale_s,      vScale_t_Comp), out[i]);
				out[i] = MAWrapper(vLTf, _mm_mul_ps(vScale_s_Comp, vScale_t     ), out[i]);
				out[i] = MAWrapper(vLBf, _mm_mul_ps(vScale_s_Comp, vScale_t_Comp), out[i]);
				out[i] = _mm_mul_ps(out[i], vNorm);
			}

			break;
		}
		case GL_DEPTH_COMPONENT:
		{
			float *pAddr = static_cast<float *>(pMipmap->mMem.addr);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS0));
			__m128 vLBf = _mm_set_ps(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS1));
			__m128 vRBf = _mm_set_ps(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT1, _mm_set1_epi32(pMipmap->mWidth), vS0));
			__m128 vLTf = _mm_set_ps(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			_mm_store_si128((__m128i *)addr, MAWrapper(vT1, _mm_set1_epi32(pMipmap->mWidth), vS1));
			__m128 vRTf = _mm_set_ps(pAddr[addr[3]], pAddr[addr[2]], pAddr[addr[1]], pAddr[addr[0]]);

			out[0] = _mm_mul_ps(vRTf, _mm_mul_ps(vScale_s,     vScale_t     ));
			out[0] = MAWrapper(vRBf, _mm_mul_ps(vScale_s,      vScale_t_Comp), out[0]);
			out[0] = MAWrapper(vLTf, _mm_mul_ps(vScale_s_Comp, vScale_t     ), out[0]);
			out[0] = MAWrapper(vLBf, _mm_mul_ps(vScale_s_Comp, vScale_t_Comp), out[0]);

			break;
		}
		default:
		{
			break;
		}
	}
}


bool Texture::ValidateState()
{
	if(!IsComplete())
		return false;

	if(mSO.eMagFilter == GL_LINEAR &&
	  (mSO.eMinFilter == GL_NEAREST_MIPMAP_NEAREST ||
	   mSO.eMinFilter == GL_NEAREST_MIPMAP_LINEAR))
		// Pixel: Texel = 1: sqrt(2)
		mMagMinThresh = 0.5f;
	else
		// Pixel: Texel = 1: 1
		mMagMinThresh = 0.0f;

	return true;
}


TextureMachine::TextureMachine():
	mNameSpace("Texture"),
	mActiveTextureUnit(0),
	mDefaultTexture(TEXTURE_TARGET_UNBOUND)
{
	mDefaultTexture.setName(0);

	for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
	{
		for(int j = 0; j < TEXTURE_TARGET_MAX; j++)
			mBoundTexture[i][j].mTex = &mDefaultTexture;
	}
}

TextureMachine::~TextureMachine()
{
	for(int j = 0; j < MAX_TEXTURE_UNITS; j++)
	{
		for(int k = 0; k < TEXTURE_TARGET_MAX; k++)
		{
			TextureBindingPoint *pBP = &mBoundTexture[j][k];

			if(pBP->mTex != &mDefaultTexture)
				pBP->mTex->DecRef();
		}
	}
}

void TextureMachine::GenTextures(GLContext *gc, int n, unsigned *textures)
{
	GLSP_UNREFERENCED_PARAM(gc);

	if(n < 0)
		return;

	mNameSpace.genNames(n, textures);
}

void TextureMachine::DeleteTextures(GLContext *gc, int n, const unsigned *textures)
{
	for(int i = 0; i < n; i++)
	{
		Texture *pTex = static_cast<Texture *>(mNameSpace.retrieveObject(textures[i]));

		if(pTex)
		{
			for(int j = 0; j < MAX_TEXTURE_UNITS; j++)
			{
				for(int k = 0; k < TEXTURE_TARGET_MAX; k++)
				{
					TextureBindingPoint *pBP = &mBoundTexture[j][k];

					if(pBP->mTex == pTex)
					{
						pBP->mTex->DecRef();
						pBP->mTex = &mDefaultTexture;
					}
				}
			}

			mNameSpace.removeObject(pTex);
			pTex->DecRef();
		}
	}

	mNameSpace.deleteNames(n, textures);
}

void TextureMachine::BindTexture(GLContext *const gc, unsigned target, unsigned texture)
{
	TextureBindingPoint *pBP = getBindingPoint(mActiveTextureUnit, target);

	if(!pBP)
		return;

	Texture *pTex;

	if(!texture)
	{
		pTex = &mDefaultTexture;
	}
	else
	{
		if(!mNameSpace.validate(texture))
			return;

		pTex = static_cast<Texture *>(mNameSpace.retrieveObject(texture));

		if(!pTex)
		{
			pTex = new Texture(target);
			pTex->setName(texture);
			mNameSpace.insertObject(pTex);
		}
		pTex->IncRef();
	}

	if (pBP->mTex != &mDefaultTexture)
		pBP->mTex->DecRef();

	pBP->mTex = pTex;
}

static bool TexImage2DValidateParams(unsigned target, int internalformat, unsigned format, unsigned type)
{
	if (internalformat == GL_DEPTH_COMPONENT || format == GL_DEPTH_COMPONENT)
	{
		if (target != GL_TEXTURE_2D)
		{
			return false;
		}

		if (format != (unsigned)internalformat)
		{
			return false;
		}

		if (type != GL_FLOAT)
		{
			return false;
		}
	}

	return true;
}

void TextureMachine::TexImage2D(GLContext *gc, unsigned target, int level,
				int internalformat, int width, int height,
				int border, unsigned format, unsigned type,
				const void *pixels)
{
	GLSP_UNREFERENCED_PARAM(gc);

	if (TexImage2DValidateParams(target, internalformat, format, type) == false)
		return;

	TextureBindingPoint *pBP = getBindingPoint(mActiveTextureUnit, target);

	if(!pBP)
		return;

	Texture *pTex = pBP->mTex;

	if(pTex->getName() == 0)
		return;

	pTex->TexImage2D(level, internalformat,
					 width, height, border,
					 format, type, pixels);
}

void TextureMachine::TexParameteri(GLContext *gc, unsigned target, unsigned pname, int param)
{
	GLSP_UNREFERENCED_PARAM(gc);

	TextureBindingPoint *pBP = getBindingPoint(mActiveTextureUnit, target);

	if(!pBP)
		return;

	Texture *pTex = pBP->mTex;

	if(pTex->getName() == 0)
		return;

	pTex->TexParameteri(pname, param);
}

void TextureMachine::TexParameterfv(GLContext *gc, unsigned target, unsigned pname, const GLfloat *params)
{
	GLSP_UNREFERENCED_PARAM(gc);

	TextureBindingPoint *pBP = getBindingPoint(mActiveTextureUnit, target);

	if(!pBP)
		return;

	Texture *pTex = pBP->mTex;

	if(pTex->getName() == 0)
		return;

	pTex->TexParameterfv(pname, params);
}

bool TextureMachine::validateTextureState(Shader *pVS, Shader *pFS, DrawContext *dc)
{
	assert(pVS && pFS);

	// TODO: add support for other targets
	if(pVS->HasSampler())
	{
		int num = pVS->getSamplerNum();

		for(int i = 0; i < num; ++i)
		{
			unsigned unit = pVS->getSamplerUnitID(i);

			Texture *pTex = mBoundTexture[unit][TEXTURE_TARGET_2D].mTex;

			if(pTex == &mDefaultTexture || !pTex->ValidateState())
				return false;

			pVS->SetupTextureInfo(unit, pTex);
		}
	}

	memset(dc->mTextures, 0, sizeof(dc->mTextures));
	if(pFS->HasSampler())
	{
		int num = pFS->getSamplerNum();

		for(int i = 0; i < num; ++i)
		{
			unsigned unit = pFS->getSamplerUnitID(i);

			Texture *pTex = mBoundTexture[unit][TEXTURE_TARGET_2D].mTex;

			if(pTex == &mDefaultTexture || !(pTex->ValidateState()))
				return false;

			// OPT: use std::shared_ptr?
			dc->mTextures[unit] = pTex;
			pTex->IncRef();
		}
	}

	return true;
}

TextureBindingPoint* TextureMachine::getBindingPoint(unsigned unit,  unsigned target)
{
	TextureTarget tt = TargetToIndex(target);

	if(tt == TEXTURE_TARGET_UNBOUND)
		return NULL;

	return &mBoundTexture[unit][tt];
}

void TextureMachine::ActiveTexture(GLContext *gc, unsigned texture)
{
	GLSP_UNREFERENCED_PARAM(gc);

	mActiveTextureUnit = texture - GL_TEXTURE0;
}

unsigned char TextureMachine::IsTexture(GLContext *, unsigned texture)
{
	if(mNameSpace.validate(texture))
		return GL_TRUE;
	else
		return GL_FALSE;
}

Texture* TextureMachine::GetBoundTexture(unsigned target, unsigned texture)
{
	Texture *pTex = static_cast<Texture *>(mNameSpace.retrieveObject(texture));

	if (pTex->GetTarget() == target)
		return pTex;
	else
		return nullptr;
}

} // namespace glsp
