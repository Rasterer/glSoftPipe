#include "PrimitiveAssembler.h"
#include "DataFlow.h"
#include "DrawEngine.h"

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
	vsOutput_v &out = bat->mVsOut;
	IBuffer_v &index = bat->mIndexBuf;
	PrimBatch &pb = bat->mPrim;
	IBuffer_v::iterator it = index.begin();

	assert(index.size() / 3 == 0);
	pb.resize(index.size() / 3);

	PrimBatch::iterator iter = pb.begin();

	while(it != index.end())
	{
		Primitive &tri = *iter;

		tri.mType = Primitive::TRIANGLE;
		tri.mVert[0] = out[*(it + 0)];
		tri.mVert[1] = out[*(it + 1)];
		tri.mVert[2] = out[*(it + 2)];

		it += 3;
		iter++;
	}

	vsOutput_v().swap(out);
	IBuffer_v().swap(bat->mIndexBuf);
}

void PrimitiveAssembler::finalize()
{
}
