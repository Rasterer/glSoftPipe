#pragma once

#include "DataFlow.h"


NS_OPEN_GLSP_OGL()


class OwnershipTester: public PipeStage
{
public:
	OwnershipTester() = default;
	~OwnershipTester() = default;

private:
	inline bool onOwnershipTesting(const Fsio &fsio);
};

class ScissorTester: public PipeStage
{
public:
	ScissorTester() = default;
	~ScissorTester() = default;

private:
	inline bool onScissorTesting(const Fsio &fsio);
};

class StencilTester: public PipeStage
{
public:
	StencilTester() = default;
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
	virtual void finalize();

private:
	inline bool onDepthTesting(const Fsio &fsio);
};

class Blender: public PipeStage
{
public:
	Blender() = default;
	~Blender() = default;

private:
	inline bool onBlending(const Fsio &fsio);
};

class Dither: public PipeStage
{
public:
	Dither() = default;
	~Dither() = default;

private:
	inline bool onDithering(const Fsio &fsio);
};


NS_CLOSE_GLSP_OGL()