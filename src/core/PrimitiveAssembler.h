#pragma once

#include "PipeStage.h"

class Batch;

class PrimitiveAssembler: public PipeStage
{
public:
	PrimitiveAssembler();
	void assemble(Batch *bat);
	virtual void emit(void *data);
	virtual void finalize();
};
