#pragma once

#include <cstdint>

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

	uint32_t mLevel;
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

	// FIXME: now just check base mipmap level
	// need check more stuff.
	bool IsComplete()
	{
		return (mIsComplete = mpMipmap->mResident);
	}

	uint32_t getLevelNum() const { return mNumMipmaps; }
	TextureMipmap*  getMipmap(uint32_t layer, int32_t level);

	const SamplerObject&  getSamplerObject() const { return mSO; }
	void TexParameteri(unsigned pname, int param);

	void TexImage2D(int level, int internalformat,
					int width, int height, int border,
					unsigned format, unsigned type, const void *pixels);

private:
	TextureMipmap  *mpMipmap;
	uint32_t		mNumMipmaps;
	TextureState	mState;
	SamplerObject	mSO;

	TextureTarget	mTextureTarget;
	uint32_t		mNumLayers;

	bool	mIsComplete;

private:
	static uint32_t FormatToGroupSize(int32_t target);
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
