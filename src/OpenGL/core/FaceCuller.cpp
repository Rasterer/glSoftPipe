#include "FaceCuller.h"
#include "DataFlow.h"
#include "DrawEngine.h"
#include "utils.h"

using glm::vec4;

NS_OPEN_GLSP_OGL()

FaceCuller::FaceCuller():
	PipeStage("Face Culling", DrawEngine::getDrawEngine()),
	mOrient(CCW),
	mCullFace(BACK)
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
	Primlist &pl = bat->mPrims;
	Primlist::iterator it = pl.begin();

	while(it != pl.end())
	{
		orient_t orient = (it->mAreaReciprocal > 0)? CCW: CW;
		face_t face = (mOrient == orient)? FRONT: BACK;

		if((mCullFace & face) != 0)
		{
			it = pl.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void FaceCuller::finalize()
{
}

NS_CLOSE_GLSP_OGL()
