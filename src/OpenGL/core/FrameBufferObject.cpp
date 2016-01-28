#include "FrameBufferObject.h"

#include "GLContext.h"
#include "Texture.h"


namespace glsp {
#include "khronos/GL/glcorearb.h"

GLAPI GLboolean APIENTRY glIsFramebuffer (GLuint framebuffer)
{
	__GET_CONTEXT();
	return gc->mFBOM.IsFramebuffer(gc, framebuffer);
}

GLAPI void APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer)
{
	__GET_CONTEXT();
	gc->mFBOM.BindFramebuffer(gc, target, framebuffer);
}

GLAPI void APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
	__GET_CONTEXT();
	gc->mFBOM.DeleteFramebuffers(gc, n, framebuffers);
}

GLAPI void APIENTRY glGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
	__GET_CONTEXT();
	gc->mFBOM.GenFramebuffers(gc, n, framebuffers);
}

GLAPI GLenum APIENTRY glCheckFramebufferStatus (GLenum target)
{
	// TODO:
	return GL_FRAMEBUFFER_COMPLETE;
}

GLAPI void APIENTRY glFramebufferTexture1D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{

}

GLAPI void APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	__GET_CONTEXT();
	gc->mFBOM.FramebufferTexture2D(gc, target, attachment, textarget, texture, level);
}

GLAPI void APIENTRY glFramebufferTexture3D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{

}

GLAPI void APIENTRY glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{

}

GLAPI void APIENTRY glGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params)
{

}

GLAPI void APIENTRY glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{

}

GLAPI void APIENTRY glRenderbufferStorageMultisample (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{

}

GLAPI void APIENTRY glFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{

}

GLAPI void APIENTRY glDrawBuffer (GLenum buf)
{
	__GET_CONTEXT();
	gc->mFBOM.SetReadDrawBuffers(gc, 1, &buf, true);
}

GLAPI void APIENTRY glReadBuffer (GLenum src)
{
	__GET_CONTEXT();
	gc->mFBOM.SetReadDrawBuffers(gc, 1, &src, false);
}

GLAPI void APIENTRY glDrawBuffers (GLsizei n, const GLenum *bufs)
{
	__GET_CONTEXT();
	gc->mFBOM.SetReadDrawBuffers(gc, n, bufs, true);
}

FrameBufferObjectMachine::FrameBufferObjectMachine():
	mReadFBO(&mDefaultFBO),
	mDrawFBO(&mDefaultFBO)
{
	mDefaultFBO.setName(0);
	mDefaultFBO.SetReadDrawBuffers(1 << GLSP_BACK_LEFT, true,  false);
	mDefaultFBO.SetReadDrawBuffers(1 << GLSP_BACK_LEFT, false, false);
}

FrameBufferObjectMachine::~FrameBufferObjectMachine()
{
	if (mReadFBO != &mDefaultFBO)
		mReadFBO->DecRef();

	if (mDrawFBO != &mDefaultFBO)
		mDrawFBO->DecRef();
}

void FrameBufferObjectMachine::GenFramebuffers(GLContext *, int n, unsigned *framebuffers)
{
	if(n > 0)
		mNameSpace.genNames(n, framebuffers);
}

void FrameBufferObjectMachine::DeleteFramebuffers(GLContext *, int n, const unsigned *framebuffers)
{
	for(int i = 0; i < n; i++)
	{
		FrameBufferObject *pFBO = static_cast<FrameBufferObject *>(mNameSpace.retrieveObject(framebuffers[i]));
		if(pFBO)
		{
			if (mReadFBO == pFBO)
			{
				pFBO->DecRef();
				mReadFBO = &mDefaultFBO;
			}

			if (mDrawFBO == pFBO)
			{
				pFBO->DecRef();
				mDrawFBO = &mDefaultFBO;
			}

			mNameSpace.removeObject(pFBO);
			pFBO->DecRef();
		}
	}

	mNameSpace.deleteNames(n, framebuffers);
}

void FrameBufferObjectMachine::BindFramebuffer(GLContext *gc, unsigned target, unsigned framebuffer)
{
	bool bBindReadFBO = false;
	bool bBindDrawFBO = false;

	switch (target)
	{
		case GL_FRAMEBUFFER:
		{
			bBindReadFBO = true;
			bBindDrawFBO = true;
			break;
		}
		case GL_DRAW_FRAMEBUFFER:
		{
			bBindDrawFBO = true;
			break;
		}
		case GL_READ_FRAMEBUFFER:
		{
			bBindReadFBO = true;
			break;
		}
		default:
			return;
	}

	FrameBufferObject *pFBO;

	if(!framebuffer)
	{
		pFBO = &mDefaultFBO;
		if (bBindReadFBO && mReadFBO != &mDefaultFBO)
		{
			mReadFBO->DecRef();
			mReadFBO = &mDefaultFBO;
		}
		if (bBindDrawFBO && mDrawFBO != &mDefaultFBO)
		{
			mDrawFBO->DecRef();
			mDrawFBO = &mDefaultFBO;
		}
	}
	else
	{
		if(!mNameSpace.validate(framebuffer))
		{
			GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "BindFramebuffer: invalid name %d!\n", framebuffer);
			return;
		}

		pFBO = static_cast<FrameBufferObject *>(mNameSpace.retrieveObject(framebuffer));

		if(!pFBO)
		{
			pFBO = new FrameBufferObject();
			pFBO->setName(framebuffer);
			mNameSpace.insertObject(pFBO);
		}
	}

	assert(pFBO);

	if (bBindReadFBO)
	{
		if (mReadFBO != &mDefaultFBO)
			mReadFBO->DecRef();

		mReadFBO = pFBO;
		if (pFBO != &mDefaultFBO)
			pFBO->IncRef();
	}

	if (bBindDrawFBO)
	{
		if (mDrawFBO != &mDefaultFBO)
			mDrawFBO->DecRef();

		mDrawFBO = pFBO;
		if (pFBO != &mDefaultFBO)
			pFBO->IncRef();
	}
}

unsigned char FrameBufferObjectMachine::IsFramebuffer(GLContext *, unsigned framebuffer)
{
	if (mNameSpace.validate(framebuffer))
		return GL_TRUE;
	else
		return GL_FALSE;
}

static FBOAttachment GLAttachmentToInternal(unsigned attachment, bool is_default)
{
	FBOAttachment inter_attach = GLSP_INVALID_ATTACHMENT;

	if (is_default)
	{
		switch (attachment)
		{
			case GL_FRONT_LEFT:
			case GL_FRONT_RIGHT:
			case GL_BACK_LEFT:
			case GL_BACK_RIGHT:
			{
				inter_attach = static_cast<FBOAttachment>((int)GLSP_FRONT_LEFT + (attachment - GL_FRONT_LEFT));
				break;
			}
			case GL_DEPTH:
			{
				inter_attach = GLSP_DEPTH;
				break;
			}
			case GL_STENCIL:
			{
				inter_attach = GLSP_STENCIL;
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else
	{
		switch (attachment)
		{
			case GL_COLOR_ATTACHMENT0:
			case GL_COLOR_ATTACHMENT1:
			case GL_COLOR_ATTACHMENT2:
			case GL_COLOR_ATTACHMENT3:
			case GL_COLOR_ATTACHMENT4:
			case GL_COLOR_ATTACHMENT5:
			case GL_COLOR_ATTACHMENT6:
			case GL_COLOR_ATTACHMENT7:
			case GL_COLOR_ATTACHMENT8:
			case GL_COLOR_ATTACHMENT9:
			case GL_COLOR_ATTACHMENT10:
			case GL_COLOR_ATTACHMENT11:
			case GL_COLOR_ATTACHMENT12:
			case GL_COLOR_ATTACHMENT13:
			case GL_COLOR_ATTACHMENT14:
			case GL_COLOR_ATTACHMENT15:
			{
				inter_attach = static_cast<FBOAttachment>((int)GLSP_COLOR_ATTACHMENT0 + (attachment - GL_COLOR_ATTACHMENT0));
				break;
			}
			case GLSP_DEPTH_ATTACHMENT:
			{
				inter_attach = GLSP_DEPTH_ATTACHMENT;
				break;
			}
			case GLSP_STENCIL_ATTACHMENT:
			{
				inter_attach = GLSP_STENCIL_ATTACHMENT;
				break;
			}
			default:
			{
				break;
			}
		}
	}

	return inter_attach;
}

void FrameBufferObjectMachine::FramebufferTexture2D(GLContext *gc, unsigned target, unsigned attachment, unsigned textarget, unsigned texture, int level)
{
	FrameBufferObject *pFBO;

	switch (target)
	{
		case GL_FRAMEBUFFER:
		case GL_DRAW_FRAMEBUFFER:
		{
			pFBO = mDrawFBO;
			break;
		}
		case GL_READ_FRAMEBUFFER:
		{
			pFBO = mReadFBO;
			break;
		}
		default:
			return;
	}

	if (pFBO == &mDefaultFBO)
		return;

	FBOAttachment attach = GLAttachmentToInternal(attachment, false);
	if (attach == GLSP_INVALID_ATTACHMENT)
		return;

	if (texture != GL_TEXTURE_2D)
		return;

	Texture *pTex;

	if (texture == 0)
	{
		pTex = nullptr;
	}
	else
	{
		pTex = gc->mTM.GetBoundTexture(textarget, texture);

		if (!pTex)
			return;
	}

	pFBO->FramebufferTexture2D(attach, pTex, level);
}

bool FrameBufferObjectMachine::ValidateFramebufferStatus(GLContext *gc)
{
	return mDrawFBO->ValidateFramebufferStatus(gc);
}

void FrameBufferObjectMachine::SetReadDrawBuffers(GLContext *, int n, const unsigned *bufs, bool draw)
{
	if (!bufs)
		return;

	int previous_mask;
	FrameBufferObject *pFBO;
	if (draw)
	{
		pFBO = mDrawFBO;
		previous_mask = pFBO->GetDrawMask();
	}
	else
	{
		pFBO = mReadFBO;
		previous_mask = pFBO->GetReadMask();
	}

	pFBO->SetReadDrawBuffers(0, draw, false);

	bool is_default = (pFBO == &mDefaultFBO);
	for (int i = 0; i < n; ++i)
	{
		FBOAttachment attach = GLAttachmentToInternal(bufs[i], is_default);

		if (attach == GLSP_INVALID_ATTACHMENT)
		{
			pFBO->SetReadDrawBuffers(previous_mask, draw, false);
			return;
		}

		pFBO->SetReadDrawBuffers(1 << attach, draw, true);
	}
}


FrameBufferObject::FrameBufferObject():
	mReadMask(1 << GLSP_COLOR_ATTACHMENT0),
	mDrawMask(1 << GLSP_COLOR_ATTACHMENT0)
{
	memset(mAttachPoints, 0, sizeof(mAttachPoints));
}

FrameBufferObject::~FrameBufferObject()
{
	if (getName() == 0)
	{
		if (mRenderTarget.pColorBuffer)
			free(mRenderTarget.pColorBuffer);

		if (mRenderTarget.pDepthBuffer)
			free(mRenderTarget.pDepthBuffer);

		if (mRenderTarget.pStencilBuffer)
			free(mRenderTarget.pStencilBuffer);
	}
	else
	{
		for (int attachment = (int)GLSP_COLOR_ATTACHMENT0; attachment < (int)GLSP_MAX_ATTACHMENTS; ++attachment)
		{
			FBOAttachPoint &attach_point = mAttachPoints[attachment];
			if (attach_point.attachment)
			{
				if (attach_point.type == FBO_ATTACHMENT_TEXTURE)
				{
					TextureMipmap* pCurrentMipmap = static_cast<TextureMipmap *>(attach_point.attachment);
					pCurrentMipmap->mpTex->DecRef();
					pCurrentMipmap->mRefCount--;
				}
				else
				{
					// TODO: render buffer
				}
			}
		}
	}
}

// TODO: render synchronization
void FrameBufferObject::FramebufferTexture2D(FBOAttachment attach, Texture *tex, int level)
{
	TextureMipmap *pMipmap = tex->getMipmap(0, level);
	if (!pMipmap)
		return;

	if (attach == GLSP_DEPTH_ATTACHMENT && tex->GetFormat() != GL_DEPTH_COMPONENT)
		return;

	FBOAttachPoint &attach_point = mAttachPoints[attach];
	if (attach_point.attachment)
	{
		if (attach_point.type == FBO_ATTACHMENT_TEXTURE)
		{
			TextureMipmap* pCurrentMipmap = static_cast<TextureMipmap *>(attach_point.attachment);
			pCurrentMipmap->mpTex->DecRef();
			pCurrentMipmap->mRefCount--;
		}
		else
		{
			// TODO: render buffer
		}
	}

	attach_point.attachment = pMipmap;
	attach_point.type = FBO_ATTACHMENT_TEXTURE;
}

bool FrameBufferObject::ValidateFramebufferStatus(GLContext *gc)
{
	if (getName() == 0)
	{
		if (mDrawMask & (1 << GLSP_BACK_LEFT))
			gc->mRT = mRenderTarget;
		else
			;// TODO:
	}
	else
	{
		int width = 0, height = 0;
		for (int attachment = (int)GLSP_COLOR_ATTACHMENT0; attachment < (int)GLSP_MAX_ATTACHMENTS; ++attachment)
		{
			FBOAttachPoint &attach_point = mAttachPoints[attachment];
			if (attach_point.attachment)
			{
				if (attach_point.type == FBO_ATTACHMENT_TEXTURE)
				{
					TextureMipmap* pCurrentMipmap = static_cast<TextureMipmap *>(attach_point.attachment);

					if (width != 0 && width != pCurrentMipmap->mWidth)
					{
						width = 0;
						break;
					}

					if (height != 0 && height != pCurrentMipmap->mWidth)
					{
						height = 0;
						break;
					}

					width  = pCurrentMipmap->mWidth;
					height = pCurrentMipmap->mHeight;
				}
				else
				{
					// TODO: render buffer
				}
			}
		}

		if (width == 0 || height == 0)
			return false;

		gc->mRT.width  = width;
		gc->mRT.height = height;

		// TODO: Multi render targets support
		FBOAttachPoint &color_attachment = mAttachPoints[GLSP_COLOR_ATTACHMENT0];
		if (mDrawMask & (1 << GLSP_COLOR_ATTACHMENT0) && color_attachment.attachment)
		{
			if (color_attachment.type == FBO_ATTACHMENT_TEXTURE)
			{
				TextureMipmap *pMipmap = static_cast<TextureMipmap *>(color_attachment.attachment);
				gc->mRT.format = pMipmap->mpTex->GetFormat();
				gc->mRT.pColorBuffer = pMipmap->mMem.addr;
			}
			else
			{
				// TODO: render buffer
			}
		}
		else
		{
			gc->mRT.pColorBuffer = nullptr;
			// TODO:
		}

		FBOAttachPoint &depth_attachment  = mAttachPoints[GLSP_DEPTH_ATTACHMENT];
		if (depth_attachment.attachment)
		{
			if (depth_attachment.type == FBO_ATTACHMENT_TEXTURE)
			{
				TextureMipmap *pMipmap = static_cast<TextureMipmap *>(depth_attachment.attachment);
				gc->mRT.pDepthBuffer = static_cast<float *>(pMipmap->mMem.addr);
			}
			else
			{
				// TODO: render buffer
			}
		}
		else
		{
			// TODO:
			gc->mRT.pDepthBuffer = nullptr;
		}

		FBOAttachPoint &stencil_attachment = mAttachPoints[GLSP_STENCIL_ATTACHMENT];
		if (stencil_attachment.attachment)
		{
			if (stencil_attachment.type == FBO_ATTACHMENT_TEXTURE)
			{
				TextureMipmap *pMipmap = static_cast<TextureMipmap *>(stencil_attachment.attachment);
				gc->mRT.pStencilBuffer = pMipmap->mMem.addr;
			}
			else
			{
				// TODO: render buffer
			}
		}
		else
		{
			// TODO:
			gc->mRT.pStencilBuffer = nullptr;
		}
	}

	return true;
}

void FrameBufferObject::DefaultFBOInitRenderTarget(int width, int height, int format)
{
	bool need_recreate = false;
	if (mRenderTarget.width != width || mRenderTarget.height != height || mRenderTarget.format != format)
		need_recreate = true;

	mRenderTarget.width  = width;
	mRenderTarget.height = height;
	mRenderTarget.format = format;

	if (need_recreate)
	{
		if (mRenderTarget.pColorBuffer)
		{
			free(mRenderTarget.pColorBuffer);
			mRenderTarget.pColorBuffer = nullptr;
		}

		if (mRenderTarget.pDepthBuffer)
		{
			free(mRenderTarget.pDepthBuffer);
			mRenderTarget.pDepthBuffer = nullptr;
		}

		if (mRenderTarget.pStencilBuffer)
		{
			free(mRenderTarget.pStencilBuffer);
			mRenderTarget.pStencilBuffer = nullptr;
		}
	}

	// TODO: get bpp from buffer format
	if (!mRenderTarget.pColorBuffer)
		mRenderTarget.pColorBuffer = malloc(width * height * 4);

	if (!mRenderTarget.pDepthBuffer)
		mRenderTarget.pDepthBuffer = (float *)malloc(width * height * sizeof(float));

	// TODO: impl stencil buffer
	mRenderTarget.pStencilBuffer = nullptr;
}

void FrameBufferObject::SetReadDrawBuffers(int mask, bool draw, bool append)
{
	if (draw)
	{
		if (append)
			mDrawMask |= mask;
		else
			mDrawMask = mask;
	}
	else
	{
		if (append)
			mReadMask |= mask;
		else
			mReadMask = mask;
	}
}

} // namespace glsp
