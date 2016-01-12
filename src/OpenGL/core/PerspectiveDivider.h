#pragma once

#include "PipeStage.h"


namespace glsp {

class Batch;

class PerspectiveDivider: public PipeStage
{
public:
	PerspectiveDivider();
	virtual ~PerspectiveDivider() { }

	virtual void emit(void *data);
	virtual void finalize();

private:
	void dividing(Batch *bat);

private:
};

} // namespace glsp
