#pragma once

#include "PipeStage.h"

NS_OPEN_GLSP_OGL()

class Batch;

class ScreenMapper: public PipeStage
{
public:
	ScreenMapper();
	virtual ~ScreenMapper() { }

	virtual void emit(void *data);
	virtual void finalize();

	void setViewPort(int x, int y, int w, int h);
	void getViewPort(int &x, int &y, int &w, int &h);

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

NS_CLOSE_GLSP_OGL()