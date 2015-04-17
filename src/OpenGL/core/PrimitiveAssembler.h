#pragma once

#include "PipeStage.h"

class Batch;

class PrimitiveAssembler: public PipeStage
{
public:
	PrimitiveAssembler();

	virtual void emit(void *data);
	virtual void finalize();

private:
	void assemble(Batch *bat);
};
