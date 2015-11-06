#pragma once

#include "glsp_defs.h"
#include "Shader.h"


NS_OPEN_GLSP_OGL()

class Interpolater;
class DrawContext;


class Rasterizer: public PipeStage
{
public:
	Rasterizer();
	virtual ~Rasterizer() = default;

	virtual void emit(void *data);
	virtual void finalize();

	void SetInterpolater(Interpolater *interp)
	{
		mpInterpolate = interp;
	}

	Interpolater* GetInterpolater() const { return mpInterpolate; }

protected:
	virtual void onRasterizing(DrawContext *dc) = 0;

	// Used to calculate gradiences.
	Interpolater *mpInterpolate;
};


/* Optimized incremental interpolate */

class Interpolater: public PipeStage
{
public:
	Interpolater();
	virtual ~Interpolater() = default;

	// PipeStage interfaces
	virtual void emit(void *data);
	virtual void finalize();

	virtual void onInterpolating(const fsInput &in,
								 const fsInput &gradX,
								 const fsInput &gradY,
								 float stepx, float stepy,
								 fsInput &out) = 0;

	// step 1 in X or Y axis
	virtual void onInterpolating(const fsInput &in,
								 const fsInput &grad,
								 float x,
								 fsInput &out) = 0;
};


NS_CLOSE_GLSP_OGL()
