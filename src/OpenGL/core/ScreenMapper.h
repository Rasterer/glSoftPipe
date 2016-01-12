#pragma once

#include "PipeStage.h"


namespace glsp {

class Batch;

class ScreenMapper: public PipeStage
{
public:
	ScreenMapper();
	virtual ~ScreenMapper() { }

	virtual void emit(void *data);
	virtual void finalize();

private:
	void viewportTransform(Batch *bat);

private:
	struct {
		unsigned int x;
		unsigned int y;
		unsigned int w;
		unsigned int h;
	} mViewport;
};

} // namespace glsp
