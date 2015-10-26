#pragma once

#include <common/glsp_defs.h>
#include "PipeStage.h"


NS_OPEN_GLSP_OGL()

class Batch;

class Binning: public PipeStage
{
public:
	Binning();
	virtual ~Binning() { }

	virtual void emit(void *data);
	virtual void finalize();

private:
	void onBinning(Batch *bat);
};

NS_CLOSE_GLSP_OGL()
