#include "PrimitiveAssembler.h"

#include "DataFlow.h"
#include "DrawEngine.h"
#include "MemoryPool.h"


namespace glsp {

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
	vsInput_v    &in    = bat->mVertexCache;
	vsOutput_v   &out   = bat->mVsOut;
	IBuffer_v    &index = bat->mIndexBuf;
	Primlist     &pl    = bat->mPrims;

	assert(index.size() % 3 == 0);

	// Free the memory in Batch.mVertexCache to avoid large memory occupy
	vsInput_v().swap(in);

	for(auto it = index.begin(); it != index.end(); it += 3)
	{
		Primitive *prim = new(MemoryPoolMT::get()) Primitive();

		prim->mType		= Primitive::TRIANGLE;
		prim->mVertNum	= 3;
		prim->mVert[0]	= out[*(it + 0)];
		prim->mVert[1]	= out[*(it + 1)];
		prim->mVert[2]	= out[*(it + 2)];
		prim->mDC       = bat->mDC;

		pl.push_back(prim);
	}

	vsOutput_v().swap(out);
	IBuffer_v().swap(bat->mIndexBuf);
}

void PrimitiveAssembler::finalize()
{
}

} // namespace glsp
