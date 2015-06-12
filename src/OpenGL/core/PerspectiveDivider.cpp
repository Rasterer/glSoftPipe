#include "PerspectiveDivider.h"
#include "DataFlow.h"
#include "DrawEngine.h"

NS_OPEN_GLSP_OGL()

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
	Primlist &in = bat->mPrims;

	for(auto it = in.begin(); it != in.end(); ++it)
	{
		for(size_t i = 0; i < 3; ++i)
		{
			vec4 &pos = it->mVert[i].position();
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

NS_CLOSE_GLSP_OGL()
