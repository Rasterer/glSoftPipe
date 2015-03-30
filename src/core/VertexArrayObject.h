#pragma once

#include "glcorearb.h"
#include "NameSpace.h"
#include "BufferObject.h"

#define MAX_VERTEX_ATTRIBS 16

struct VertexAttribState
{
	VertexAttribState();
	int	mCompSize;
	int mCompNum;
	int	mStride;
	unsigned	mOffset;
	BufferObject	*mBO;
};

struct VertexArrayObject: public NameItem
{
	VertexArrayObject();
	unsigned	mAttribEnables;
	BufferObject	*mBoundElementBuffer;
	VertexAttribState	mAttribState[MAX_VERTEX_ATTRIBS];
};

class VAOMachine
{
public:
	VAOMachine();
	void VertexAttribPointer(GLContext *gc, unsigned index, int size, unsigned type, bool normalized, int stride, const void *pointer);
	void GenVertexArrays(GLContext *gc, int n, unsigned *arrays);
	void DeleteVertexArrays(GLContext *gc, int n, unsigned *arrays);
	void BindVertexArray(GLContext *gc, unsigned array);
	void EnableVertexArrayAttrib(GLContext *gc, unsigned vaobj, unsigned index);
	void DisableVertexArrayAttrib(GLContext *gc, unsigned vaobj, unsigned index);

private:
	NameSpace	mNameSpace;
	VertexArrayObject	*mActiveVAO;
	VertexArrayObject	mDefaultVAO;
};
