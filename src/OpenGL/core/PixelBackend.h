#pragma once

#include "glsp_defs.h"
#include "PipeStage.h"
#include "DrawEngine.h"


NS_OPEN_GLSP_OGL()

class Fsio;
struct RenderTarget;

// FIXME: move to a global class
extern const RenderTarget *gRT;

class OwnershipTester: public PipeStage
{
public:
	OwnershipTester();
	~OwnershipTester() = default;

private:
	inline bool onOwnershipTesting(const Fsio &fsio);
};

class ScissorTester: public PipeStage
{
public:
	ScissorTester();
	~ScissorTester() = default;

private:
	inline bool onScissorTesting(const Fsio &fsio);
};

class StencilTester: public PipeStage
{
public:
	StencilTester();
	~StencilTester() = default;

private:
	inline bool onStencilTesting(const Fsio &fsio);
};

class ZTester: public PipeStage
{
public:
	ZTester();
	~ZTester() = default;

	// PipeStage interfaces
	virtual void emit(void *data);

private:
	inline bool onDepthTesting(const Fsio &fsio);
	bool onDepthTestingSIMD(Fsiosimd &fsio);
};

class Blender: public PipeStage
{
public:
	Blender();
	~Blender() = default;

	// PipeStage interfaces
	virtual void emit(void *data);

private:
	inline void onBlending(Fsio &fsio);
};

class Dither: public PipeStage
{
public:
	Dither();
	~Dither() = default;

private:
	inline bool onDithering(const Fsio &fsio);
};

/* The final stage */
class FBWriter: public PipeStage
{
public:
	FBWriter();
	~FBWriter() = default;

	virtual void emit(void *data);

private:
	inline void onFBWriting(const Fsio &fsio);
	void onFBWritingSIMD(const Fsiosimd &fsio);
};

NS_CLOSE_GLSP_OGL()
