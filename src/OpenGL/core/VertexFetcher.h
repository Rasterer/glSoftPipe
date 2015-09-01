#pragma once

#include <common/glsp_defs.h>
#include "Shader.h"
#include "PipeStage.h"


NS_OPEN_GLSP_OGL()

class DrawContext;
class GLContext;

// TODO: tunning
// Set this macro to a big enough number to disable multi batch dispatching.
#define VERTEX_CACHE_EMIT_THRESHHOLD 12

class VertexFetcher: public PipeStage
{
public:
	VertexFetcher();
	virtual ~VertexFetcher() {}
protected:
	// Should be called from PipeStage's emit() method
	virtual void fetchVertex(DrawContext *dc) = 0;
};

class VertexCachedFetcher: public VertexFetcher
{
public:
	VertexCachedFetcher();
	virtual ~VertexCachedFetcher() = default;

	virtual void emit(void *data);
	virtual void finalize();

protected:
	virtual void fetchVertex(DrawContext *dc);
	void dispatchOneBatch(Batch *bat);
};

NS_CLOSE_GLSP_OGL()
