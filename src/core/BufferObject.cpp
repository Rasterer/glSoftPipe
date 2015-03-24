#include <iostream>
#include "BufferObject.h"
#include "glsp_defs.h"

GLAPI void APIENTRY glGenBuffers (GLsizei n, GLuint *buffers)
{
	__GET_GC();
	gc->mBOM->GenBuffers(gc, n, buffers); 
}

GLAPI void APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
	__GET_GC();
	gc->mBOM->DeleteBuffers(gc, n, buffers);
}

GLAPI void APIENTRY glBindBuffer (GLenum target, GLuint buffer)
{
	__GET_GC();
	gc->mBOM->BindBuffer(gc, target, buffer);
}

GLAPI void APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
	__GET_GC();
	gc->mBOM->BufferData(gc, target, buffer);
}

bool BufferObjectMachine::GenBuffers(GLContext *gc, int n, unsigned *buffers)
{
	GLSP_UNREFERENCED_PARAM(gc);

	return mNameSpace.genNames(n, buffers);
}

bool BufferObjectMachine::DeleteBuffers(GLContext *gc, int n, unsigned *buffers)
{
	GLSP_UNREFERENCED_PARAM(gc);

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

		pBO = mNameSpace.retieveObject(buffer);

		if(!pBO)
		{
			pBO = new BufferObject();
			pBO->mNameItem.mName = buffer;
			insertObject(&pBO->mNameItem);
		}

		mBindings[targetIndex].mBO = pBO;
	}

	return true;
}

bool BufferObjectMachine::BufferData(GLContext *gc, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
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

int TargetToIndex(GLenum target)
{
	switch(target)
	{
		case GL_ARRAY_BUFFER:			return ARRAY_BUFFER_INDEX;
		case GL_ELEMENT_ARRAY_BUFFER:	return ELEMENT_ARRAY_BUFFER_INDEX;
	}

	return -1;
}
