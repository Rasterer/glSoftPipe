#pragma once

#include "glcorearb.h"
#include "NameSpace.h"
#include "BufferObject.h"

#define MAX_VERTEX_ATTRIBS 16

struct VertexAttribState
{
	VertexAttribState();
	int	mSize;
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

private:
	NameSpace	mNameSpace;
	VertexArrayObject	*mActiveVAO;
	VertexArrayObject	mDefaultVAO;
};
