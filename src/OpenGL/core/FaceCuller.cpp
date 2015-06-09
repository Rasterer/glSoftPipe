#include "FaceCuller.h"
#include "DataFlow.h"
#include "DrawEngine.h"
#include "utils.h"

using glm::vec4;

NS_OPEN_GLSP_OGL()

FaceCuller::FaceCuller():
	PipeStage("Face Culling", DrawEngine::getDrawEngine()),
	mOrient(CCW),
	mCullFace(BACK),
	mCullEnabled(false)
{
}

// TODO: move emit to PipeStage base class, reload onEmit
void FaceCuller::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	culling(bat);

	getNextStage()->emit(bat);
}

// FIXME(done): consider free vsOutput memory
// OPT: get stage states from gc?
void FaceCuller::culling(Batch *bat)
{
	Primlist &in = bat->mPrims;
	Primlist::iterator it = in.begin();

	while(it != in.end())
	{
		vec4 &pos0 = it->mVert[0].position();
		vec4 &pos1 = it->mVert[1].position();
		vec4 &pos2 = it->mVert[2].position();

		float ex = pos1.x - pos0.x;
		float ey = pos1.y - pos0.y;
		float fx = pos2.x - pos0.x;
		float fy = pos2.y - pos0.y;
		float area = ex * fy - ey * fx;

		if(abs(area) > 1.0f)
		{
			orient_t orient = (area > 0)? CCW: CW;
			face_t face = (mOrient == orient)? FRONT: BACK;

			if(mCullEnabled && (mCullFace & face) != 0)
			{
				it = in.erase(it);
				continue;
			}
			else
			{
				it->mAreaReciprocal = 1.0f / area;
			}
		}
		else
		{
			it = in.erase(it);
			continue;
		}
		++it;
	}
}

void FaceCuller::finalize()
{
}

NS_CLOSE_GLSP_OGL()
