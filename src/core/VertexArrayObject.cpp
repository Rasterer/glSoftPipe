#include "VertexArrayObject.h"

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
