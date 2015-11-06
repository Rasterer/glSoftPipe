#pragma once

#include "glsp_defs.h"
#include "PipeStage.h"


NS_OPEN_GLSP_OGL()

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

NS_CLOSE_GLSP_OGL()
