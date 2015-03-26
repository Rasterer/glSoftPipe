#include "VertexArrayObject.h"

VertexArrayObject::VertexArrayObject()
{
	mAttribEnables = 0;
}

VAOMachine::VAOMachine()
{
	mDefaultVAO.mNameItem.mName = 0;
	mActiveVAO = &mDefaultVAO;
}
