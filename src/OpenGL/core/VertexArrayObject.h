#pragma once

#include "NameSpace.h"
#include "DataFlow.h"
#include "BufferObject.h"


namespace glsp {


#define MAX_VERTEX_ATTRIBS MAX_SHADER_REGISTERS

struct VertexAttribState
{
	VertexAttribState();
	int	mAttribSize;
	int mCompNum;
	int	mStride;
	unsigned long mOffset;
	BufferObject *mBO;
};

struct VertexArrayObject: public NameItem
{
	VertexArrayObject();
	unsigned	mAttribEnables;
	BindingPoint	mBoundElementBuffer;
	VertexAttribState	mAttribState[MAX_VERTEX_ATTRIBS];
};

class VAOMachine
{
public:
	VAOMachine();
	~VAOMachine();
	void VertexAttribPointer(GLContext *gc, unsigned index, int size, unsigned type, bool normalized, int stride, const void *pointer);
	void GenVertexArrays(GLContext *gc, int n, unsigned *arrays);
	void DeleteVertexArrays(GLContext *gc, int n, const unsigned *arrays);
	void BindVertexArray(GLContext *gc, unsigned array);
	void EnableVertexAttribArray(GLContext *gc, unsigned index);
	void DisableVertexAttribArray(GLContext *gc, unsigned index);
	void EnableVertexArrayAttrib(GLContext *gc, unsigned vaobj, unsigned index);
	void DisableVertexArrayAttrib(GLContext *gc, unsigned vaobj, unsigned index);
	unsigned char IsVertexArray(GLContext *gc, unsigned array);

	VertexArrayObject *getActiveVAO() const
	{
		return mActiveVAO;
	}

private:
	NameSpace	mNameSpace;
	VertexArrayObject	*mActiveVAO;
	VertexArrayObject	 mDefaultVAO;
};

} // namespace glsp
