#include "PerspectiveDivider.h"
#include "DataFlow.h"
#include "DrawEngine.h"

PerspectiveDivider::PerspectiveDivider():
	PipeStage("Perspective Dividing", DrawEngine::getDrawEngine())
{
}

void PerspectiveDivider::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	dividing(bat);

	getNextStage()->emit(bat);
}

// From clip space to NDC
void PerspectiveDivider::dividing(Batch *bat)
{
#if PRIMITIVE_OWNS_VERTICES
	PrimBatch &in = bat->mPrims;

	for(PrimBatch::iterator it = in.begin(); it != in.end(); ++it)
	{
		for(size_t i = 0; i < 3; ++i)
		{
			vec4 &pos = it->mVert[i].position();
			pos.x /= pos.w;
			pos.y /= pos.w;
			pos.z /= pos.w;
		}
	}
#endif
}

void PerspectiveDivider::finalize()
{
}
