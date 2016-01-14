#pragma once

#include "Shader.h"


namespace glsp {

class Interpolater;
struct DrawContext;

class Rasterizer: public PipeStage
{
public:
	Rasterizer();
	virtual ~Rasterizer() = default;

	virtual void emit(void *data);
	virtual void finalize();

private:
	virtual void onRasterizing() = 0;
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


} // namespace glsp
