#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include "glsp_defs.h"
#include "BufferObject.h"
#include "GLContext.h"

using namespace std;

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

bool BufferObjectMachine::GenBuffers(GLContext *gc, int n, unsigned *buffers)
{
	GLSP_UNREFERENCED_PARAM(gc);

	return mNameSpace.genNames(n, buffers);
}

bool BufferObjectMachine::DeleteBuffers(GLContext *gc, int n, const unsigned *buffers)
{
	GLSP_UNREFERENCED_PARAM(gc);

	for(int i = 0; i < n; i++)
	{
		BufferObject *pBO = static_cast<BufferObject *>(mNameSpace.retrieveObject(buffers[i]));
		if(pBO)
			mNameSpace.removeObject(pBO);
	}

	return mNameSpace.deleteNames(n, buffers);
}

bool BufferObjectMachine::BindBuffer(GLContext *gc, unsigned target, unsigned buffer)
{
	BindingPoint *pBP = getBindingPoing(gc, target);

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
			cout << "BindBuffer: no such buffer " << buffer << "!" << endl;
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
	BindingPoint *pBP = getBindingPoing(gc, target);

	if(!pBP)
		return false;

	BufferObject *pBO = pBP->mBO;
	if(!pBO)
	{
		cout << "BufferData: no buffer bound " << target << "!" << endl;
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
	BindingPoint *pBP = getBindingPoing(gc, target);

	if(!pBP)
		return NULL;

	return pBP->mBO;
}

BindingPoint *BufferObjectMachine::getBindingPoing(GLContext *gc, unsigned target)
{
	int targetIndex = TargetToIndex(target);

	if(targetIndex == -1)
	{
		cout << "BindBuffer: error target " << target << "!" << endl;
		return NULL;
	}

	if(targetIndex == ELEMENT_ARRAY_BUFFER_INDEX)
		return &(gc->mVAOM.getActiveVAO()->mBoundElementBuffer);
	else
		return &(mBindings[target]);
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
