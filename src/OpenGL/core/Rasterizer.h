#pragma once

#include <glm/glm.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <common/glsp_defs.h>
#include "Shader.h"


NS_OPEN_GLSP_OGL()

class Interpolater;

/* The wrapper for the whole rasterizer stages.
 * Also responsible for resource life management.
 */
class RasterizerWrapper: public PipeStage
{
	friend class DrawEngine;

public:
	RasterizerWrapper();
	~RasterizerWrapper();

	// PipeStage interfaces
	virtual void emit(void *data);
	virtual void finalize();

	void initPipeline();
	void linkPipeStages(GLContext *gc);

private:
	PipeStage *mpRasterizer; // scan conversion
	PipeStage *mpInterpolate;
	PipeStage *mpFS;
	PipeStage *mpOwnershipTest;
	PipeStage *mpScissorTest;
	PipeStage *mpStencilTest;
	PipeStage *mpDepthTest;
	PipeStage *mpBlender;
	PipeStage *mpDither;
	PipeStage *mpFBWriter;
};

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

protected:
	virtual void onRasterizing(Batch *bat) = 0;

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

	// Should be invoked before onInterpolating
	virtual void CalculateRadiences(Gradience *pGrad) = 0;

	virtual void onInterpolating(const fsInput &in,
								 const fsInput &gradX,
								 const fsInput &gradY,
								 float stepx, float stepy,
								 fsInput &out) = 0;

	// step 1 in X or Y axis
	virtual void onInterpolating(fsInput &in,
								 const fsInput &grad) = 0;
};


NS_CLOSE_GLSP_OGL()