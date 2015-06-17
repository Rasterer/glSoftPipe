#pragma once

#include "khronos/GL/glcorearb.h"
#include <common/glsp_defs.h>
#include "NameSpace.h"


NS_OPEN_GLSP_OGL()

struct SamplerObject: public NameItem
{
	GLenum eWrapS;
	GLenum eWrapT;
	GLenum eWrapR;

	GLenum eMagFilter;
	GLenum eMinFilter;
};

NS_CLOSE_GLSP_OGL()