#include <cassert>
#include <cstdlib>
#include <cstring>

#include "BufferObject.h"
#include "GLContext.h"
#include "glsp_debug.h"


using namespace std;

namespace glsp {
#include "khronos/GL/glcorearb.h"

GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers)
{
	__GET_CONTEXT();
	gc->mBOM.GenBuffers(gc, n, buffers); 
}

GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
	__GET_CONTEXT();
	gc->mBOM.DeleteBuffers(gc, n, buffers);
}

GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer)
{
	__GET_CONTEXT();
	gc->mBOM.BindBuffer(gc, target, buffer);
}

GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
	__GET_CONTEXT();
	gc->mBOM.BufferData(gc, target, size, data, usage);
}

BufferObject::BufferObject():
	mSize(0),
	mUsage(0),
	mAddr(NULL)
{
}

BindingPoint::BindingPoint():
	mBO(NULL)
{
}

BindingPoint::~BindingPoint()
{
}

void BufferObjectMachine::GenBuffers(GLContext *gc, int n, unsigned *buffers)
{
	GLSP_UNREFERENCED_PARAM(gc);

	if(n < 0)
		return;

	mNameSpace.genNames(n, buffers);
}

bool BufferObjectMachine::DeleteBuffers(GLContext *gc, int n, const unsigned *buffers)
{
	for(int i = 0; i < n; i++)
	{
		BufferObject *pBO = static_cast<BufferObject *>(mNameSpace.retrieveObject(buffers[i]));
		if(pBO)
		{
			BindingPoint *pBP;

			pBP = getBindingPoint(gc, GL_ARRAY_BUFFER);

			if(pBP->mBO == pBO)
				pBP->mBO = NULL;

			pBP = getBindingPoint(gc, GL_ELEMENT_ARRAY_BUFFER);

			if(pBP->mBO == pBO)
				pBP->mBO = NULL;

			mNameSpace.removeObject(pBO);
			pBO->DecRef();
		}
	}

	return mNameSpace.deleteNames(n, buffers);
}

bool BufferObjectMachine::BindBuffer(GLContext *gc, unsigned target, unsigned buffer)
{
	BindingPoint *pBP = getBindingPoint(gc, target);

	if(!pBP)
		return false;

	if(!buffer)
	{
		pBP->mBO = NULL;
	}
	else
	{
		BufferObject *pBO;

		if(!mNameSpace.validate(buffer))
		{
			GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "BindBuffer: no such buffer %d!\n", buffer);
			return false;
		}

		pBO = static_cast<BufferObject *>(mNameSpace.retrieveObject(buffer));

		if(!pBO)
		{
			pBO = new BufferObject();
			pBO->setName(buffer);
			mNameSpace.insertObject(pBO);
		}

		pBP->mBO = pBO;
	}

	return true;
}

bool BufferObjectMachine::BufferData(GLContext *gc, unsigned target, unsigned size, const void *data, unsigned usage)
{
	BindingPoint *pBP = getBindingPoint(gc, target);

	if(!pBP)
		return false;

	BufferObject *pBO = pBP->mBO;
	if(!pBO)
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "BufferData: no buffer bound for target %d\n", target);
		return false;
	}

	pBO->mUsage = usage;

	if(pBO->mSize == size)
	{
		assert(pBO->mAddr);
	}
	else
	{
		// OPT: buffer pool alloc?
		free(pBO->mAddr);
		pBO->mAddr = malloc(size);
		pBO->mSize = size;
	}

	if(data)
		memcpy(pBO->mAddr, data, size);

	return true;
}

BufferObject *BufferObjectMachine::getBoundBuffer(unsigned target)
{
	__GET_CONTEXT();
	BindingPoint *pBP = getBindingPoint(gc, target);

	if(!pBP)
		return NULL;

	return pBP->mBO;
}

BindingPoint *BufferObjectMachine::getBindingPoint(GLContext *gc, unsigned target)
{
	int targetIndex = TargetToIndex(target);

	if(targetIndex == -1)
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "BindBuffer: error target %d\n", target);
		return NULL;
	}

	if(targetIndex == ELEMENT_ARRAY_BUFFER_INDEX)
		return &(gc->mVAOM.getActiveVAO()->mBoundElementBuffer);
	else
	{
		return &(mBindings[targetIndex]);
	}
}

int BufferObjectMachine::TargetToIndex(GLenum target)
{
	switch(target)
	{
		case GL_ARRAY_BUFFER:			return ARRAY_BUFFER_INDEX;
		case GL_ELEMENT_ARRAY_BUFFER:	return ELEMENT_ARRAY_BUFFER_INDEX;
	}

	return -1;
}

} // namespace glsp
