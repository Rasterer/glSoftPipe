#pragma once

#include "PipeStage.h"

class Batch;

class ScreenMapper: public PipeStage
{
public:
	ScreenMapper();
	virtual void emit(void *data);
	virtual void finalize();
	void viewportTransform(Batch *bat);

	void setViewPort(int x, int y, int w, int h);
	void getViewPort(int &x, int &y, int &w, int &h);

private:
	struct {
		unsigned int x;
		unsigned int y;
		unsigned int w;
		unsigned int h;
	} mViewPort;
};
