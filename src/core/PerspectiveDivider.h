#pragma once

#include "PipeStage.h"

class Batch;

class PerspectiveDivider: public PipeStage
{
public:
	PerspectiveDivider();
	virtual void emit(void *data);
	virtual void finalize();
	void dividing(Batch *bat);

private:
};
