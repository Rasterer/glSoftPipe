#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include <common/glsp_defs.h>
#include "SamplerObject.h"
#include "NameSpace.h"


NS_OPEN_GLSP_OGL()

#define MAX_TEXTURE_UNITS 16
#define MAX_TEXTURE_MIPMAP_LEVELS 6


class  GLContext;
struct TextureState;
struct TextureMipmap;
class  Texture;
class  TextureMachine;
class  Shader;

enum TextureTarget
{
	TEXTURE_TARGET_UNBOUND = -1,
	TEXTURE_TARGET_1D = 0,
	TEXTURE_TARGET_2D,
	TEXTURE_TARGET_3D,
	TEXTURE_TARGET_1D_ARRAY,
	TEXTURE_TARGET_2D_ARRAY,
	TEXTURE_TARGET_RECTANGLE,
	TEXTURE_TARGET_BUFFER,
	TEXTURE_TARGET_CUBE_MAP,
	TEXTURE_TARGET_CUBE_MAP_ARRAY,
	TEXTURE_TARGET_2D_MULTISAMPLE,
	TEXTURE_TARGET_2D_MULTISAMPLE_ARRAY,
	TEXTURE_TARGET_MAX
};

struct TextureState
{
// TODO:
};

struct MemoryBackup
{
	uint32_t size;
	void    *addr;
};

struct TextureMipmap
{
	TextureMipmap();
	~TextureMipmap();

	MemoryBackup mMem;
	bool mResident;

	int32_t mLevel;
	uint32_t mWidth, mHeight;
	uint32_t mBytesPerTexel;
	unsigned mFormat;

	Texture *mpTex;
};

class Texture: public NameItem
{
public:
	Texture(TextureTarget target);
	virtual ~Texture();

	bool ValidateState();

	uint32_t getLevelNum() const { return mNumMipmaps; }
	TextureMipmap*  getMipmap(uint32_t layer, int32_t level) const;
	TextureMipmap*  getBaseMipmap(uint32_t layer) const { return getMipmap(layer, 0); }

	const SamplerObject&  getSamplerObject() const { return mSO; }
	void TexParameteri(unsigned pname, int param);

	void TexImage2D(int level, int internalformat,
					int width, int height, int border,
					unsigned format, unsigned type, const void *pixels);

	float getMagMinThresh() const { return mMagMinThresh; }
	uint32_t getAvailableMipmaps() const { return mAvailableMipmaps; }

public:
	typedef void (*PFNTexture2D)(const Shader *pShader, const Texture *pTex, const glm::vec2 &coord, glm::vec4 &res);
	typedef void (*PFNTexture2DMag)(const Texture *pTex, unsigned int level, const glm::vec2 &coord, glm::vec4 &res);
	typedef void (*PFNTexture2DMin)(const Texture *pTex, float lambda, const glm::vec2 &coord, glm::vec4 &res);

	PFNTexture2D	m_pfnTexture2D;
	PFNTexture2DMag	m_pfnTexture2DMag;
	PFNTexture2DMin	m_pfnTexture2DMin;

private:
	// FIXME: now just check base mipmap level
	// need check more stuff.
	bool IsComplete();

private:
	TextureMipmap  *mpMipmap;
	uint32_t		mNumMipmaps;
	uint32_t		mAvailableMipmaps;
	TextureState	mState;
	SamplerObject	mSO;
	float 			mMagMinThresh;

	TextureTarget	mTextureTarget;
	uint32_t		mNumLayers;

	bool	mIsComplete;
};

struct TextureBindingPoint
{
	TextureBindingPoint();
	~TextureBindingPoint();

	Texture *mTex;
};

class TextureMachine
{
public:
	TextureMachine();
	virtual ~TextureMachine();

	void ActiveTexture(GLContext *gc, unsigned texture);
	void GenTextures(GLContext *gc, int n, unsigned *textures);
	void BindTexture(GLContext *gc, unsigned target, unsigned texture);
	void DeleteTextures(GLContext *gc, int n, const unsigned *textures);
	void TexParameteri(GLContext *gc, unsigned target, unsigned pname, int param);
	void TexImage2D(GLContext *gc, unsigned target, int level,
					int internalformat, int width, int height,
					int border, unsigned format, unsigned type,
					const void *pixels);

	bool validateTextureState(Shader *pVS, Shader *pFS);

private:
	TextureBindingPoint* getBindingPoint(unsigned unit, unsigned target);

private:
	NameSpace mNameSpace;

	unsigned int			mActiveTextureUnit;
	Texture					mDefaultTexture;
	TextureBindingPoint		mBoundTexture[MAX_TEXTURE_UNITS][TEXTURE_TARGET_MAX];
};

NS_CLOSE_GLSP_OGL()