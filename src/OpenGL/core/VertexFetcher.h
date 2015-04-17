#pragma once

#include "Shader.h"
#include "PipeStage.h"

class DrawContext;
class GLContext;

// Set this macro to a big enough number to disable multi batch dispatching.
#define VERTEX_CACHE_EMIT_THRESHHOLD 1000000

class VertexFetcher: public PipeStage
{
public:
	VertexFetcher();
	~VertexFetcher() {}
protected:
	// Should be called from PipeStage's emit() method
	virtual void fetchVertex(DrawContext *dc) = 0;
};

class VertexCachedFetcher: public VertexFetcher
{
public:
	VertexCachedFetcher();
	~VertexCachedFetcher() {}

	virtual void emit(void *data);
	virtual void finalize();

protected:
	virtual void fetchVertex(DrawContext *dc);
};
