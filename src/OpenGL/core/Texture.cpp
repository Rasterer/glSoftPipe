#include "Texture.h"

#include <cstring>

#include "GLContext.h"
#include "khronos/GL/glcorearb.h"


using glsp::ogl::GLContext;
using glsp::ogl::TextureMachine;

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
		case GL_TEXTURE_2D: return MAX_TEXTURE_MIPMAP_LEVELS;
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

} /* namespace */


TextureMipmap::TextureMipmap():
	mResident(false),
	mWidth(0),
	mHeight(0),
	mBytesPerTexel(0)
{
	mMem.size = 0;
	mMem.addr = NULL;
}

TextureMipmap::~TextureMipmap()
{
	if(mResident)
		free(mMem.addr);
}

Texture::Texture(TextureTarget target):
	mTextureTarget(target),
	mNumLayers(1), // FIXME: set layer num correctly
	mIsComplete(false)
{
	mSO.setName(0);
	// FIXME: the initial value should be GL_LINEAR
	// However, it hasn't been implemented yet.
	mSO.eMagFilter = GL_NEAREST;

	mNumMipmaps = TargetToMipmapLevels(target);

	mpMipmap = new TextureMipmap[mNumMipmaps];

	for(uint32_t i = 0; i < mNumMipmaps; i++)
	{
		mpMipmap[i].mLevel = i;
		mpMipmap[i].mpTex  = this;
	}
}

Texture::~Texture()
{
	if(mpMipmap)
		delete []mpMipmap;
}

// TODO: add support for other targets
TextureMipmap* Texture::getMipmap(uint32_t layer, int32_t level)
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

	assert(border == 0);

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

bool TextureMachine::validateTextureState(Shader *pVS, Shader *pFS)
{
	assert(pVS && pFS);

	// TODO: add support for other targets
	if(pVS->HasSampler())
	{
		for(int i = 0; i < pVS->getSamplerNum(); i++)
		{
			unsigned unit = pVS->getSamplerUnitID(i);

			Texture *pTex = mBoundTexture[unit][TEXTURE_TARGET_2D].mTex;

			if(pTex == &mDefaultTexture || !(pTex->IsComplete()))
				return false;

			pVS->SetupTextureInfo(unit, pTex);
		}
	}

	if(pFS->HasSampler())
	{
		for(int i = 0; i < pFS->getSamplerNum(); i++)
		{
			unsigned unit = pFS->getSamplerUnitID(i);

			Texture *pTex = mBoundTexture[unit][TEXTURE_TARGET_2D].mTex;

			if(pTex == &mDefaultTexture || !(pTex->IsComplete()))
				return false;

			pFS->SetupTextureInfo(unit, pTex);
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
