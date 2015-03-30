#include "VertexArrayObject.h"
#include "BufferObject.h"
#include "glcorearb.h"

GLAPI void APIENTRY glGenVertexArrays (GLsizei n, GLuint *arrays)
{
	__GET_CONTEXT();
	gc->mVAOM.GenVertexArrays(gc, n, arrays);
}

GLAPI void APIENTRY glDeleteVertexArrays (GLsizei n, const GLuint *arrays)
{
	__GET_CONTEXT();
	gc->mVAOM.DeleteVertexArrays(gc, n, arrays);
}

GLAPI void APIENTRY glBindVertexArray (GLuint array)
{
	__GET_CONTEXT();
	gc->mVAOM.BindVertexArray(gc, array);
}

GLAPI void APIENTRY glEnableVertexArrayAttrib (GLuint vaobj, GLuint index)
{
	__GET_CONTEXT();
	gc->mVAOM.EnableVertexArrayAttrib(gc, vaobj, index);
}

GLAPI void APIENTRY glDisableVertexArrayAttrib (GLuint vaobj, GLuint index)
{
	__GET_CONTEXT();
	gc->mVAOM.DisableVertexArrayAttrib(gc, vaobj, index);
}

GLAPI void APIENTRY glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
{
	__GET_CONTEXT();
	gc->mVAOM.VertexAttribPointer(gc, index, size, type, normalized, stride, pointer);
}

VertexAttribState::VertexAttribState():
	mSize(0),
	mStride(0),
	mOffset(0),
	mBO(NULL)
{
}

VertexArrayObject::VertexArrayObject():
	mAttribEnables(0),
	mBoundElementBuffer(NULL)
{
}

VAOMachine::VAOMachine()
{
	mDefaultVAO.setName(0);
	mActiveVAO = &mDefaultVAO;
}

void VAOMachine::GenVertexArrays(GLContext *gc, int n, unsigned *arrays)
{
	GLSP_UNREFERENCED_PARAM(gc);
	mNameSpace.GenNames(n, arrays);
}

void VAOMachine::DeleteVertexArrays(GLContext *gc, int n, unsigned *arrays)
{
	GLSP_UNREFERENCED_PARAM(gc);

	for(int i = 0; i < n; i++)
	{
		VertexArrayObject *pVAO = mNameSpace.retrieveObject(arrays[i]);

		if(pVAO)
		{
			mNameSpace.removeObject(pVAO);
		}
	}

	mNameSpace.DeleteNames(n, arrays);
}

void VAOMachine::BindVertexArray(GLContext *gc, unsigned array)
{
	GLSP_UNREFERENCED_PARAM(gc);

	if(!array)
	{
		mActiveVAO = &mDefaultVAO;
	}
	else
	{
		VertexArrayObject *pVAO;

		if(!mNameSpace.validate(array))
			return;

		pVAO = mNameSpace.retrieveObject(array);

		if(!pVAO)
		{
			pVAO = new VertexArrayObject();
			pVAO->setName(array);
			mNameSpace.insertObject(pVAO);
			mActiveVAO = pVAO;
		}
	}
}

void VAOMachine::EnableVertexArrayAttrib(GLContext *gc, unsigned vaobj, unsigned index)
{
	GLSP_UNREFERENCED_PARAM(gc);

	VertexArrayObject *pVAO = mNameSpace.retrieveObject(vaobj);

	if(!pVAO)
		return;

	pVAO->mAttribEnables |= (1 << index);
}

void VAOMachine::DisableVertexArrayAttrib(GLContext *gc, unsigned vaobj, unsigned index)
{
	GLSP_UNREFERENCED_PARAM(gc);

	VertexArrayObject *pVAO = mNameSpace.retrieveObject(vaobj);

	if(!pVAO)
		return;

	pVAO->mAttribEnables &= ~(1 << index);
}

void VAOMachine::VertexAttribPointer(
	GLContext *gc,
	unsigned index,
	int size,
	unsigned type,
	bool normalized,
	int stride,
	const void *pointer)
{
	assert(index < MAX_VERTEX_ATTRIBS);
	assert(size >= 1 && size <= 4);
	// TODO: Account for normalize and other data types
	assert(normalized == false);
	assert(type == GL_FLOAT);

	VertexArrayObject *pVAO = mActiveVAO;
	VertexAttribState &vas = pVAO->mAttribState[index];
	vas.mCompSize = sizeof(GL_FLOAT);
	vas.mCompNum = size;
	vas.mStride = stride;
	vas.mOffset = reinterpret_cast<unsigned>(pointer);
	vas.mBO = gc->mBOM.getBoundBuffer(GL_ARRAY_BUFFER);
}