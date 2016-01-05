#include "Texture.h"
#include "DrawEngine.h"

#include <cmath>
#include <cstring>

#include "GLContext.h"
#include "TBDR.h"
#include "compiler.h"


using glsp::ogl::GLContext;
using glsp::ogl::TextureMachine;
using glm::vec2;
using glm::vec4;

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

NS_OPEN_GLSP_OGL()

namespace {

typedef void (*PFNCopyTexels)(const void *pSrc, TextureMipmap *pMipmap);

#if 0
void CopyTexelIntactly(const uint8_t *pSrc, TextureMipmap *pMipmap)
{
	memcpy(pMipmap->mMem.addr,
		   pSrc,
		   pMipmap->mHeight * pMipmap->mWidth * pMipmap->mBytesPerTexel);
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
#endif

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
#if 0
bool PickupCopyFunc(int internalformat, unsigned format, unsigned type, uint32_t *bytesPerTexel, PFNCopyTexels *ppfnCopyTexels)
{
	switch(format)
	{
		case GL_RGBA:
		{
			switch(type)
			{
				case GL_UNSIGNED_BYTE:
				{
					switch(internalformat)
					{
						case GL_RGBA:
						case GL_RGBA8:
						{
							*bytesPerTexel  = 4;
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
		case GL_RGB:
		{
			switch(type)
			{
				case GL_UNSIGNED_BYTE:
				{
					switch(internalformat)
					{
						case GL_RGB:
						case GL_RGB8:
						{
							*bytesPerTexel  = 3;
							*ppfnCopyTexels = (PFNCopyTexels)CopyTexelIntactly;
						}
						case GL_RGB565:
						{
							*bytesPerTexel  = 2;
							*ppfnCopyTexels = (PFNCopyTexels)CopyTexelR8G8B8ToR5G6B5;
						}
						default:
						{
							return false;
						}
					}
				}
				case GL_UNSIGNED_SHORT_5_6_5:
				{
					switch(internalformat)
					{
						case GL_RGB:
						case GL_RGB565:
						{
							*bytesPerTexel  = 2;
							*ppfnCopyTexels = (PFNCopyTexels)CopyTexelIntactly;
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
		default:
		{
			return false;
		}
	}
}
#endif

bool PickupCopyFunc(int internalformat, unsigned format, unsigned type, uint32_t *bytesPerTexel, PFNCopyTexels *ppfnCopyTexels)
{
	GLSP_UNREFERENCED_PARAM(internalformat);

	*bytesPerTexel = 4 * sizeof(float);

	switch(format)
	{
		case GL_RGBA:
		{
			switch(type)
			{
				case GL_UNSIGNED_BYTE:
				{
					*ppfnCopyTexels = (PFNCopyTexels)CopyTexelR8G8B8A8ToRGBAF;
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
			switch(type)
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
		case TEXTURE_TARGET_2D: return MAX_TEXTURE_MIPMAP_LEVELS;
		default:			return 1;
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


float ComputeLambda(const Shader *pShader, const vec2 &coord, int texW, int texH)
{
#if 0
	const Fsio &fsio = *static_cast<Fsio *>(pShader->getPrivateData());
	const Triangle *tri = static_cast<Triangle *>(fsio.m_priv0);
	const fsInput &base = tri->mVert[0].mAttrReciprocal;
	const fsInput &derivativeX = tri->mGradientX;
	const fsInput &derivativeY = tri->mGradientY;
#if 0
	int texCoordLoc = pShader->GetTextureCoordLocation();

	float z_stepx = 1.0f / (start[0].w + derivativeX[0].w);
	float z_stepy = 1.0f / (start[0].w + derivativeY[0].w);

	float dudx = texW * ((start[texCoordLoc].x + derivativeX[texCoordLoc].x) * z_stepx - coord.x);
	float dvdx = texH * ((start[texCoordLoc].y + derivativeX[texCoordLoc].y) * z_stepx - coord.y);
	float dudy = texW * ((start[texCoordLoc].x + derivativeY[texCoordLoc].x) * z_stepy - coord.x);
	float dvdy = texH * ((start[texCoordLoc].y + derivativeY[texCoordLoc].y) * z_stepy - coord.y);

	float x = dudx * dudx + dvdx * dvdx;
	float y = dudy * dudy + dvdy * dvdy;

	return 0.5f * std::log2(std::max(x, y));
#else
	// Refer to:
	// http://www.gamasutra.com/view/feature/3301/runtime_mipmap_filtering.php?print=1
	const float stepx = fsio.x + 0.5f - base[0].x;
	const float stepy = fsio.y + 0.5f - base[0].y;

	float Z = derivativeX[0].w * stepx + derivativeY[0].w * stepy + base[0].w;
	Z = Z * Z; Z = Z * Z; // Z = pow(Z, 4)

	float ux = tri->c + tri->a * stepy;
	ux = ux * ux;

	float vx = tri->d + tri->b * stepy;
	vx = vx * vx;

	float uy = tri->e - tri->a * stepx;
	uy = uy * uy;

	float vy = tri->f - tri->b * stepx;
	vy = vy * vy;

	texW = texW * texW;
	texH = texH * texH;

	return 0.5f * std::log2(std::max(ux * texW + vx * texH, uy * texW + vy * texH) / Z);
#endif
#endif
	return 0.0f;
}

inline void BiLinearInterpolate(float scaleX, float scaleY, const vec4 &lb, const vec4 &lt, const vec4 &rb, const vec4 &rt, vec4 &res)
{
	res = lb * ((1.0f - scaleX) * (1.0f - scaleY)) +
		  lt * ((1.0f - scaleX) * scaleY) +
		  rb * (scaleX * (1.0f - scaleY)) +
		  rt * (scaleX * scaleY);
#if 0
	res = (lb *= ((1.0 - scaleX) * (1.0f - scaleY))) +
		  (lt *= ((1.0 - scaleX) * scaleY)) +
		  (rb *= (scaleX * (1.0f - scaleY))) +
		  (rt *= (scaleX * scaleY));
#endif
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

#if 0
void Sample2DNearestLevelSIMD(const Texture *pTex, unsigned int level, const vec4 &s, const vec4 &t, vec4 *res)
{
	const TextureMipmap *pMipmap = pTex->getMipmap(0, level);

	__m128i vS = _mm_cvtps_epi32(_mm_floor_ps(_mm_load_ps(&s)));
	__m128i vT = _mm_cvtps_epi32(_mm_floor_ps(_mm_load_ps(&t)));
	vS = pTex->m_pfnWrapSSIMD(vS, pMipmap->mWidth);
	vT = pTex->m_pfnWrapTSIMD(vT, pMipmap->mHeight);

	if(i < 0 || i >= (int)pMipmap->mWidth ||
	   j < 0 || j >= (int)pMipmap->mHeight)
	{
		res = pTex->getSamplerObject().mBorderColor;
		return;
	}

	vec4 *pAddr = static_cast<vec4 *>(pMipmap->mMem.addr);

	res = pAddr[j * pMipmap->mWidth + i];
}
#endif

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
		lvl = std::ceil(lambda + 0.5f) - 1;

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
		lvl = std::ceil(lambda + 0.5f) - 1;

	if(lvl > pTex->getAvailableMipmaps())
		lvl = pTex->getAvailableMipmaps();

	Sample2DLinearLevel(pTex, lvl, coord, res);
}

void Sample2DNearestMipmapLinear(const Texture *pTex, float lambda, const vec2 &coord, vec4 &res)
{
	uint32_t lvl1, lvl2;

	lvl1 = std::floor(lambda);

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

	lvl1 = std::floor(lambda);

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
#if 0
void Sample2DNearestSIMD(const Texture *pTex, const glm::vec4 &s, const glm::vec4 &t, glm::vec4 *res)
{
	Sample2DNearestLevelSIMD(pTex, 0, s, t, res);
}
#endif

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

template <class T>
inline T clamp(T val, T l, T h)
{
	return (val < l) ? l: (val > h) ? h: val;
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

#if 0
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
	mBytesPerTexel(0),
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

Texture::Texture(TextureTarget target):
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
		case TEXTURE_TARGET_2D:
			assert(layer == 0);
			return &mpMipmap[level];
		default:
			return NULL;
	}
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

	uint32_t bytesPerTexel = 0;
	PFNCopyTexels pfnCopyTexels = NULL;

	if(!PickupCopyFunc(internalformat, format, type, &bytesPerTexel, &pfnCopyTexels))
	{
		return;
	}

	uint32_t size = width * height * bytesPerTexel;
	bool bNeedRealloc = true;

	TextureMipmap *pMipmap = getMipmap(layer, level);
	pMipmap->mFormat = internalformat;
	pMipmap->mBytesPerTexel = bytesPerTexel;

	if(pMipmap->mResident)
	{
		if(size != pMipmap->mMem.size)
		{
			free(pMipmap->mMem.addr);
		}
		else
		{
			// Size match, luckily, we can reuse current backup memory.
			bNeedRealloc = false;
		}
	}
	else
	{
		pMipmap->mResident = true;
	}

	if(bNeedRealloc)
		pMipmap->mMem.addr = malloc(size);

	// NULL pointer, just alloc memory and leave it uninitialized.
	// TODO: add PBO support.
	if(!pixels)
		return;

	pMipmap->mWidth  = width;
	pMipmap->mHeight = height;

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
	bool ret = true;

	for(uint32_t i = 0; i <= mAvailableMipmaps; ++i)
	{
		if(!mpMipmap[i].mResident)
		{
			ret = false;
			break;
		}
	}

	mIsComplete = true;

	return ret;
}

bool Texture::PickupWrapFunc(SamplerObject &so)
{
	switch(so.eWrapS)
	{
		case GL_CLAMP_TO_EDGE:
			m_pfnWrapS     = &WrapClampToEdge;
			//m_pfnWrapSSIMD = &WrapClampToEdgeSIMD;
			break;
		case GL_CLAMP_TO_BORDER:
			m_pfnWrapS     = &WrapClampToBorder;
			//m_pfnWrapSSIMD = &WrapClampToBorderSIMD;
			break;
		case GL_REPEAT:
			m_pfnWrapS     = &WrapRepeat;
			//m_pfnWrapSSIMD = &WrapRepeatSIMD;
			break;
		case GL_MIRRORED_REPEAT:
			m_pfnWrapS     = &WrapMirroredRepeat;
			//m_pfnWrapSSIMD = &WrapMirroredRepeatSIMD;
			break;
		case GL_MIRROR_CLAMP_TO_EDGE:
			m_pfnWrapS     = &WrapMirrorClampToEdge;
			//m_pfnWrapSSIMD = &WrapMirrorClampToEdgeSIMD;
			break;
		default:
			return false;
	}

	switch(so.eWrapT)
	{
		case GL_CLAMP_TO_EDGE:
			m_pfnWrapT     = &WrapClampToEdge;
			//m_pfnWrapTSIMD = &WrapClampToEdgeSIMD;
			break;
		case GL_CLAMP_TO_BORDER:
			m_pfnWrapT     = &WrapClampToBorder;
			//m_pfnWrapTSIMD = &WrapClampToBorderSIMD;
			break;
		case GL_REPEAT:
			m_pfnWrapT     = &WrapRepeat;
			//m_pfnWrapTSIMD = &WrapRepeatSIMD;
			break;
		case GL_MIRRORED_REPEAT:
			m_pfnWrapT     = &WrapMirroredRepeat;
			//m_pfnWrapTSIMD = &WrapMirroredRepeatSIMD;
			break;
		case GL_MIRROR_CLAMP_TO_EDGE:
			m_pfnWrapT     = &WrapMirrorClampToEdge;
			//m_pfnWrapTSIMD = &WrapMirrorClampToEdgeSIMD;
			break;
		default:
			return false;
	}

	switch(so.eWrapR)
	{
		case GL_CLAMP_TO_EDGE:
			m_pfnWrapR     = &WrapClampToEdge;
			//m_pfnWrapRSIMD = &WrapClampToEdgeSIMD;
			break;
		case GL_CLAMP_TO_BORDER:
			m_pfnWrapR     = &WrapClampToBorder;
			//m_pfnWrapRSIMD = &WrapClampToBorderSIMD;
			break;
		case GL_REPEAT:
			m_pfnWrapR     = &WrapRepeat;
			//m_pfnWrapRSIMD = &WrapRepeatSIMD;
			break;
		case GL_MIRRORED_REPEAT:
			m_pfnWrapR     = &WrapMirroredRepeat;
			//m_pfnWrapRSIMD = &WrapMirroredRepeatSIMD;
			break;
		case GL_MIRROR_CLAMP_TO_EDGE:
			m_pfnWrapR     = &WrapMirrorClampToEdge;
			//m_pfnWrapRSIMD = &WrapMirrorClampToEdgeSIMD;
			break;
		default:
			return false;
	}

	return true;
}

static inline __m128i MAWrapper(const __m128i &v, const __m128i &stride, const __m128i &h)
{
#if defined(__AVX2__)
	// FMA, relaxed floating-point precision
	return _mm_fmadd_epi32(v, stride, h);
#else
	return _mm_add_epi32(_mm_mullo_epi32(v, stride), h);
#endif
}

static inline __m128 MAWrapper(const __m128 &v, const __m128 &stride, const __m128 &h)
{
#if defined(__AVX2__)
	// FMA, relaxed floating-point precision
	return _mm_fmadd_ps(v, stride, h);
#else
	return _mm_add_ps(_mm_mul_ps(v, stride), h);
#endif
}

void Texture::Texture2DSIMD(const __m128 &s, const __m128 &t, __m128 res[])
{
	const TextureMipmap *pMipmap = getMipmap(0, 0);

	__m128 vTexSpaceS = MAWrapper(s, _mm_set1_ps(pMipmap->mWidth), _mm_set1_ps(-0.5f));
	__m128 vTexSpaceT = MAWrapper(t, _mm_set1_ps(pMipmap->mHeight), _mm_set1_ps(-0.5f));
	__m128i vS0 = _mm_cvtps_epi32(_mm_floor_ps(vTexSpaceS));
	__m128i vT0 = _mm_cvtps_epi32(_mm_floor_ps(vTexSpaceT));
	__m128i vS1 = _mm_add_epi32(vS0, _mm_set1_epi32(1));
	__m128i vT1 = _mm_add_epi32(vT0, _mm_set1_epi32(1));

	vS0 = _mm_min_epi32(_mm_max_epi32(vS0, _mm_setzero_si128()), _mm_set1_epi32(pMipmap->mWidth - 1));
	vT0 = _mm_min_epi32(_mm_max_epi32(vT0, _mm_setzero_si128()), _mm_set1_epi32(pMipmap->mHeight - 1));
	vS1 = _mm_min_epi32(_mm_max_epi32(vS1, _mm_setzero_si128()), _mm_set1_epi32(pMipmap->mWidth - 1));
	vT1 = _mm_min_epi32(_mm_max_epi32(vT1, _mm_setzero_si128()), _mm_set1_epi32(pMipmap->mHeight - 1));

	__m128 *pAddr = static_cast<__m128 *>(pMipmap->mMem.addr);
	int __attribute__((aligned(16))) addr[4];

	_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS0));
	__m128 &vLB0 = pAddr[addr[0]];
	__m128 &vLB1 = pAddr[addr[1]];
	__m128 &vLB2 = pAddr[addr[2]];
	__m128 &vLB3 = pAddr[addr[3]];

	_mm_store_si128((__m128i *)addr, MAWrapper(vT1, _mm_set1_epi32(pMipmap->mWidth), vS0));
	__m128 &vLT0 = pAddr[addr[0]];
	__m128 &vLT1 = pAddr[addr[1]];
	__m128 &vLT2 = pAddr[addr[2]];
	__m128 &vLT3 = pAddr[addr[3]];

	_mm_store_si128((__m128i *)addr, MAWrapper(vT0, _mm_set1_epi32(pMipmap->mWidth), vS1));
	__m128 &vRB0 = pAddr[addr[0]];
	__m128 &vRB1 = pAddr[addr[1]];
	__m128 &vRB2 = pAddr[addr[2]];
	__m128 &vRB3 = pAddr[addr[3]];

	_mm_store_si128((__m128i *)addr, MAWrapper(vT1, _mm_set1_epi32(pMipmap->mWidth), vS1));
	__m128 &vRT0 = pAddr[addr[0]];
	__m128 &vRT1 = pAddr[addr[1]];
	__m128 &vRT2 = pAddr[addr[2]];
	__m128 &vRT3 = pAddr[addr[3]];

	float __attribute__((aligned(16))) scale_s[4];
	float __attribute__((aligned(16))) scale_t[4];
	_mm_store_ps(scale_s, _mm_sub_ps(vTexSpaceS, _mm_floor_ps(vTexSpaceS)));
	_mm_store_ps(scale_t, _mm_sub_ps(vTexSpaceT, _mm_floor_ps(vTexSpaceT)));

	res[0] = _mm_mul_ps(vRT0, _mm_set_ps1(scale_s[0] * scale_t[0]));
	res[0] = MAWrapper(vRB0, _mm_set_ps1(scale_s[0] * (1.0f - scale_t[0])), res[0]);
	res[0] = MAWrapper(vLT0, _mm_set_ps1((1.0f - scale_s[0]) * scale_t[0]), res[0]);
	res[0] = MAWrapper(vLB0, _mm_set_ps1((1.0f - scale_s[0]) * (1.0f - scale_t[0])), res[0]);

	res[1] = _mm_mul_ps(vRT1, _mm_set_ps1(scale_s[1] * scale_t[1]));
	res[1] = MAWrapper(vRB1, _mm_set_ps1(scale_s[1] * (1.0f - scale_t[1])), res[1]);
	res[1] = MAWrapper(vLT1, _mm_set_ps1((1.0f - scale_s[1]) * scale_t[1]), res[1]);
	res[1] = MAWrapper(vLB1, _mm_set_ps1((1.0f - scale_s[1]) * (1.0f - scale_t[1])), res[1]);

	res[2] = _mm_mul_ps(vRT2, _mm_set_ps1(scale_s[2] * scale_t[2]));
	res[2] = MAWrapper(vRB2, _mm_set_ps1(scale_s[2] * (1.0f - scale_t[2])), res[2]);
	res[2] = MAWrapper(vLT2, _mm_set_ps1((1.0f - scale_s[2]) * scale_t[2]), res[2]);
	res[2] = MAWrapper(vLB2, _mm_set_ps1((1.0f - scale_s[2]) * (1.0f - scale_t[2])), res[2]);

	res[3] = _mm_mul_ps(vRT3, _mm_set_ps1(scale_s[3] * scale_t[3]));
	res[3] = MAWrapper(vRB3, _mm_set_ps1(scale_s[3] * (1.0f - scale_t[3])), res[3]);
	res[3] = MAWrapper(vLT3, _mm_set_ps1((1.0f - scale_s[3]) * scale_t[3]), res[3]);
	res[3] = MAWrapper(vLB3, _mm_set_ps1((1.0f - scale_s[3]) * (1.0f - scale_t[3])), res[3]);
}

bool Texture::ValidateState()
{
	if(!IsComplete())
		return false;

	if(mSO.eMagFilter == mSO.eMinFilter)
	{
		switch(mSO.eMagFilter)
		{
			case GL_NEAREST:
				m_pfnTexture2D     = &Sample2DNearest;
				//m_pfnTexture2DSIMD = &Sample2DNearestSIMD;
				break;
			case GL_LINEAR:
				m_pfnTexture2D     = &Sample2DLinear;
				//m_pfnTexture2DSIMD = &Sample2DLinearSIMD;
				break;
			default:
				return false;
		}
	}
	else
	{
		if(mSO.eMagFilter == GL_LINEAR &&
		  (mSO.eMinFilter == GL_NEAREST_MIPMAP_NEAREST ||
		   mSO.eMinFilter == GL_NEAREST_MIPMAP_LINEAR))
			// Pixel: Texel = 1: sqrt(2)
			mMagMinThresh = 0.5f;
		else
			// Pixel: Texel = 1: 1
			mMagMinThresh = 0.0f;

		switch(mSO.eMagFilter)
		{
			case GL_NEAREST:
				m_pfnTexture2DMag     = &Sample2DNearestLevel;
				//m_pfnTexture2DMagSIMD = &Sample2DNearestLevelSIMD;
				break;
			case GL_LINEAR:
				m_pfnTexture2DMag     = &Sample2DLinearLevel;
				//m_pfnTexture2DMagSIMD = &Sample2DLinearLevelSIMD;
				break;
			default:
				return false;
		}

		switch(mSO.eMinFilter)
		{
			case GL_NEAREST:
				m_pfnTexture2DMin = &Sample2DNearestNonmipmap;
				//m_pfnTexture2DMinSIMD = &Sample2DNearestNonmipmapSIMD;
				break;
			case GL_LINEAR:
				m_pfnTexture2DMin     = &Sample2DLinearNonmipmap;
				//m_pfnTexture2DMinSIMD = &Sample2DLinearNonmipmapSIMD;
				break;
			case GL_NEAREST_MIPMAP_NEAREST:
				m_pfnTexture2DMin = &Sample2DNearestMipmapNearest;
				//m_pfnTexture2DMinSIMD = &Sample2DNearestMipmapNearestSIMD;
				break;
			case GL_NEAREST_MIPMAP_LINEAR:
				m_pfnTexture2DMin = &Sample2DNearestMipmapLinear;
				//m_pfnTexture2DMinSIMD = &Sample2DNearestMipmapLinearSIMD;
				break;
			case GL_LINEAR_MIPMAP_NEAREST:
				m_pfnTexture2DMin = &Sample2DLinearMipmapNearest;
				//m_pfnTexture2DMinSIMD = &Sample2DLinearMipmapNearestSIMD;
				break;
			case GL_LINEAR_MIPMAP_LINEAR:
				m_pfnTexture2DMin = &Sample2DLinearMipmapLinear;
				//m_pfnTexture2DMinSIMD = &Sample2DLinearMipmapLinearSIMD;
				break;
			default:
				return false;
		}

		m_pfnTexture2D     = &Sample2DLambda;
		//m_pfnTexture2DSIMD = &Sample2DLambdaSIMD;
	}

	bool ret = PickupWrapFunc(mSO);

	return ret;
}

TextureBindingPoint::TextureBindingPoint():
	mTex(NULL)
{
}

TextureBindingPoint::~TextureBindingPoint()
{
}

TextureMachine::TextureMachine():
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
						pBP->mTex = &mDefaultTexture;
				}
			}

			mNameSpace.removeObject(pTex);
			delete pTex;
		}
	}

	mNameSpace.deleteNames(n, textures);
}

void TextureMachine::BindTexture(GLContext *const gc, unsigned target, unsigned texture)
{
	TextureBindingPoint *pBP = getBindingPoint(mActiveTextureUnit, target);

	if(!pBP)
		return;

	if(!texture)
	{
		pBP->mTex = &mDefaultTexture;
	}
	else
	{
		if(!mNameSpace.validate(texture))
			return;

		Texture *pTex = static_cast<Texture *>(mNameSpace.retrieveObject(texture));

		if(!pTex)
		{
			pTex = new Texture(TargetToIndex(target));
			pTex->setName(texture);
			mNameSpace.insertObject(pTex);
		}

		pBP->mTex = pTex;
	}
}

void TextureMachine::TexImage2D(GLContext *gc, unsigned target, int level,
				int internalformat, int width, int height,
				int border, unsigned format, unsigned type,
				const void *pixels)
{
	GLSP_UNREFERENCED_PARAM(gc);

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

	if(pFS->HasSampler())
	{
		int num = pFS->getSamplerNum();

		for(int i = 0; i < num; ++i)
		{
			unsigned unit = pFS->getSamplerUnitID(i);

			Texture *pTex = mBoundTexture[unit][TEXTURE_TARGET_2D].mTex;

			if(pTex == &mDefaultTexture || !(pTex->ValidateState()))
				return false;

			dc->mTextures[unit] = *pTex;
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

NS_CLOSE_GLSP_OGL()
