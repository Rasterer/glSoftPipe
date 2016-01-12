#pragma once

#include "PipeStage.h"

namespace glsp {

class Batch;

class PrimitiveAssembler: public PipeStage
{
public:
	PrimitiveAssembler();
	virtual ~PrimitiveAssembler() { }

	virtual void emit(void *data);
	virtual void finalize();

private:
	void assemble(Batch *bat);
};

} // namespace glsp
