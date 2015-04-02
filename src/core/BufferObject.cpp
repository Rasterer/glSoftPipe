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
	int targetIndex = TargetToIndex(target);

	GLSP_UNREFERENCED_PARAM(gc);

	if(targetIndex == -1)
	{
		cout << "BindBuffer: error target " << target << "!" << endl;
		return false;
	}

	if(!buffer)
	{
		mBindings[targetIndex].mBO = NULL;
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

		mBindings[targetIndex].mBO = pBO;
	}

	return true;
}

bool BufferObjectMachine::BufferData(GLContext *gc, unsigned target, unsigned size, const void *data, unsigned usage)
{
	int targetIndex = TargetToIndex(target);

	GLSP_UNREFERENCED_PARAM(gc);

	BufferObject *pBO = mBindings[targetIndex].mBO;
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
	int targetIndex = TargetToIndex(target);

	return mBindings[targetIndex].mBO;
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
