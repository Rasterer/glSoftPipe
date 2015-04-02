#pragma once

#include "Shader.h"
#include "PipeStage.h"

class DrawContext;
class GLContext;

// Set this macro to a big enough number to disable multi batch dispatching.
#define VERTEX_CACHE_EMIT_THRESHHOLD 1000000

class VertexCachedAssembler: public PipeStage
{
public:
	VertexCachedAssembler();
	virtual void emit(void *data);
	virtual void finalize();
	void assembleVertex(DrawContext *dc, GLContext *gc);
};
