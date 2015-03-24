#include "glcorearb.h"
#include "NameSpace.h"

#define ARRAY_BUFFER_INDEX			0
#define ELEMENT_ARRAY_BUFFER_INDEX	1

#define MAX_BUFOBJ_BINDINGS 2

struct BufferObject
{
	BufferObject():
		mSize(0),
		mUsage(0),
		mAddr(NULL)
	{
	}

	NameItem mNameItem;
	size_t mSize;
	unsigned mUsage;
	void *mAddr;
};

struct BindingPoint
{
	BindingPoint():
		mBO(NULL)
	{
	}

	BufferObject *mBO;
};

class BufferObjectMachine
{
public:
	bool GenBuffers(GLContext *gc, int n, unsigned *buffers);
	bool DeleteBuffers(GLContext *gc, int n, unsigned *buffers);
	bool BindBuffer(GLContext *gc, unsigned target, unsigned buffer);
	bool BufferData(GLContext *gc, unsigned target, int size, const void *data, unsigned usage);

private:
	int TargetToIndex(unsigned target);
	NameSpace mNameSpace;
	BindingPoint mBindings[MAX_BUFOBJ_BINDINGS];
};
