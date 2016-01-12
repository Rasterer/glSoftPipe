#pragma once

#include "NameSpace.h"


namespace glsp {
#include "khronos/GL/glcorearb.h"

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

} // namespace glsp
