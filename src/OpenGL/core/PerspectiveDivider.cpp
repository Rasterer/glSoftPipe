#include "PerspectiveDivider.h"
#include "DataFlow.h"
#include "DrawEngine.h"

namespace glsp {


using glm::vec4;
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
	for (Primitive *prim: bat->mPrims)
	{
		for(size_t i = 0; i < 3; ++i)
		{
			vec4 &pos = prim->mVert[i].position();
			const float ZReciprocal = 1.0f / pos.w;

			pos.x *= ZReciprocal;
			pos.y *= ZReciprocal;
			pos.z *= ZReciprocal;
		}
	}
}

void PerspectiveDivider::finalize()
{
}

} // namespace glsp
