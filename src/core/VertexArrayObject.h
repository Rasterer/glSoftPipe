#pragma once

#include "glcorearb.h"
#include "NameSpace.h"

#define MAX_VERTEX_ATTRIBS 16

struct VertexAttribState
{
	int	mSize;
	int	mstride;
	unsigned	mOffset;
	BufferObject	*mBO;
};

struct VertexArrayObject
{
	VertexArrayObject();
	NameItem	mNameItem;
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
