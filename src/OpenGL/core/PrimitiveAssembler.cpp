#include "PrimitiveAssembler.h"
#include "DataFlow.h"
#include "DrawEngine.h"


NS_OPEN_GLSP_OGL()

PrimitiveAssembler::PrimitiveAssembler():
	PipeStage("Primitive Assembly", DrawEngine::getDrawEngine())
{
}

// TODO: impl point/line assembly
void PrimitiveAssembler::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);

	assemble(bat);

	// TODO: free Batch.mIndex memory?
	getNextStage()->emit(bat);
}

// OPT: which is better
// 1. Primitive refs to the vertex(smart pointer or alloc pool?)
// 2. Primitive owns vertex
void PrimitiveAssembler::assemble(Batch *bat)
{
	vsOutput_v   &out   = bat->mVsOut;
	IBuffer_v    &index = bat->mIndexBuf;
	Primlist     &pl    = bat->mPrims;

	assert(index.size() % 3 == 0);
	pl.resize(index.size() / 3);
	Primlist::iterator iter = pl.begin();

	for(auto it = index.begin(); it != index.end(); it += 3)
	{
		Primitive &tri = *iter;

		tri.mType		= Primitive::TRIANGLE;
		tri.mVertNum	= 3;
		tri.mVert[0]	= out[*(it + 0)];
		tri.mVert[1]	= out[*(it + 1)];
		tri.mVert[2]	= out[*(it + 2)];
		tri.mDC         = bat->mDC;

		++iter;
	}

	vsOutput_v().swap(out);
	IBuffer_v().swap(bat->mIndexBuf);
}

void PrimitiveAssembler::finalize()
{
}

NS_CLOSE_GLSP_OGL()
