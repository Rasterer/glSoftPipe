#pragma once

#include "NameSpace.h"


namespace glsp {

class Texture;
class GLContext;

enum FrameBufferType
{
	FRAME_BUFFER_READ = 0,
	FRAME_BUFFER_DRAW,
	MAX_FRAME_BUFFER_TYPES
};

enum FBOAttachment
{
	GLSP_COLOR_ATTACHMENT0     = 0,
	GLSP_COLOR_ATTACHMENT1     = 1,
	GLSP_COLOR_ATTACHMENT2     = 2,
	GLSP_COLOR_ATTACHMENT3     = 3,
	GLSP_COLOR_ATTACHMENT4     = 4,
	GLSP_COLOR_ATTACHMENT5     = 5,
	GLSP_COLOR_ATTACHMENT6     = 6,
	GLSP_COLOR_ATTACHMENT7     = 7,
	GLSP_COLOR_ATTACHMENT8     = 8,
	GLSP_COLOR_ATTACHMENT9     = 9,
	GLSP_COLOR_ATTACHMENT10    = 10,
	GLSP_COLOR_ATTACHMENT11    = 11,
	GLSP_COLOR_ATTACHMENT12    = 12,
	GLSP_COLOR_ATTACHMENT13    = 13,
	GLSP_COLOR_ATTACHMENT14    = 14,
	GLSP_COLOR_ATTACHMENT15    = 15,
	GLSP_MAX_COLOR_ATTACHMENTS = 16,
	GLSP_DEPTH_ATTACHMENT      = 16,
	GLSP_STENCIL_ATTACHMENT    = 17,
	GLSP_MAX_ATTACHMENTS       = 18,
	GLSP_INVALID_ATTACHMENT    = 0xFFFFFFFF,

	// Default fbo
	GLSP_FRONT_LEFT      = 0,
	GLSP_FRONT_RIGHT     = 1,
	GLSP_BACK_LEFT     = 2,
	GLSP_BACK_RIGHT    = 3,
	GLSP_DEPTH          = 4,
	GLSP_STENCIL        = 5,
#if 0
	// alias
	GLSP_BACK           = 0,
	GLSP_FRONT          = 2,
	GLSP_LEFT           = 2,
	GLSP_FRONT_AND_BACK = 2,
	GLSP_RIGHT          = 3,
#endif
};

enum FBOAttachmentType
{
	FBO_ATTACHMENT_TEXTURE,
	FBO_ATTACHMENT_RENDERBUFFER
};

struct FBOAttachPoint
{
	void *attachment;
	int type;
};

struct RenderTarget
{
	int width;
	int height;
	int format;
	void  *pColorBuffer;
	float *pDepthBuffer;
	void  *pStencilBuffer;
};

class FrameBufferObject: public NameItem
{
public:
	FrameBufferObject();
	~FrameBufferObject();

	void FramebufferTexture2D(FBOAttachment attach, Texture *tex, int level);
	void SetReadDrawBuffers(int mask, bool draw, bool append);
	int  GetReadMask() const { return mReadMask; }
	int  GetDrawMask() const { return mDrawMask; }

	void DefaultFBOInitRenderTarget(int width, int height, int format);
	bool ValidateFramebufferStatus(GLContext *gc);

private:
	union {
		FBOAttachPoint mAttachPoints[GLSP_MAX_ATTACHMENTS];
		// Used for default render target
		RenderTarget   mRenderTarget;
	};

	int mReadMask;
	int mDrawMask;
};

class FrameBufferObjectMachine
{
public:
	FrameBufferObjectMachine();
	~FrameBufferObjectMachine();

	unsigned char IsFramebuffer(GLContext *gc, unsigned framebuffer);
	void GenFramebuffers(GLContext *gc, int n, unsigned *framebuffers);
	void DeleteFramebuffers(GLContext *gc, int n, const unsigned *framebuffers);
	void BindFramebuffer(GLContext *gc, unsigned target, unsigned framebuffer);
	void FramebufferTexture2D(GLContext *gc, unsigned target, unsigned attachment, unsigned textarget, unsigned texture, int level);
	void SetReadDrawBuffers(GLContext *gc, int n, const unsigned *bufs, bool draw);

	bool ValidateFramebufferStatus(GLContext *gc);

	FrameBufferObject *GetDrawFBO() const { return mDrawFBO; }
	FrameBufferObject *GetReadFBO() const { return mReadFBO; }
	FrameBufferObject *GetDefaultFBO() { return &mDefaultFBO; }

private:
	NameSpace          mNameSpace;
	FrameBufferObject *mReadFBO;
	FrameBufferObject *mDrawFBO;
	FrameBufferObject  mDefaultFBO;
};

} // namespace glsp
