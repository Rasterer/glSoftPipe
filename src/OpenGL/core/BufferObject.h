#pragma once

#include "NameSpace.h"

namespace glsp {

#define ARRAY_BUFFER_INDEX			0
#define ELEMENT_ARRAY_BUFFER_INDEX	1

#define MAX_BUFOBJ_BINDINGS 2

class GLContext;

struct BufferObject: public NameItem
{
	BufferObject();

	unsigned	mSize;
	unsigned	mUsage;
	void	*mAddr;
};

struct BindingPoint
{
	BufferObject *mBO;
};

class BufferObjectMachine
{
public:
	BufferObjectMachine();
	~BufferObjectMachine();

	void GenBuffers(GLContext *gc, int n, unsigned *buffers);
	bool DeleteBuffers(GLContext *gc, int n, const unsigned *buffers);
	bool BindBuffer(GLContext *gc, unsigned target, unsigned buffer);
	bool BufferData(GLContext *gc, unsigned target, unsigned size, const void *data, unsigned usage);
	unsigned char IsBuffer(GLContext *gc, unsigned buffer);
	BufferObject *getBoundBuffer(unsigned target);

private:
	BindingPoint *getBindingPoint(GLContext *gc, unsigned target);
	int TargetToIndex(unsigned target);
	NameSpace mNameSpace;
	BindingPoint mBindings[MAX_BUFOBJ_BINDINGS];
};

} // namespace glsp
