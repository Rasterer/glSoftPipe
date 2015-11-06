#pragma once

#include "khronos/GL/glcorearb.h"
#include "glsp_defs.h"
#include "NameSpace.h"


NS_OPEN_GLSP_OGL()

struct SamplerObject: public NameItem
{
	SamplerObject():
		eWrapS(GL_REPEAT),
		eWrapT(GL_REPEAT),
		eWrapR(GL_REPEAT),
		eMagFilter(GL_LINEAR),
		eMinFilter(GL_NEAREST_MIPMAP_LINEAR)
	{
	}
	GLenum eWrapS;
	GLenum eWrapT;
	GLenum eWrapR;

	GLenum eMagFilter;
	GLenum eMinFilter;
};

NS_CLOSE_GLSP_OGL()
