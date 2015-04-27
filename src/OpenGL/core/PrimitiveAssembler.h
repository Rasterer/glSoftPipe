#pragma once

#include "PipeStage.h"

NS_OPEN_GLSP_OGL()

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

NS_CLOSE_GLSP_OGL()
