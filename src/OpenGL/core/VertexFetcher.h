#pragma once

#include "Shader.h"
#include "PipeStage.h"


namespace glsp {

struct DrawContext;
class GLContext;

class VertexFetcher: public PipeStage
{
public:
	VertexFetcher();
	virtual ~VertexFetcher() {}
protected:
	// Should be called from PipeStage's emit() method
	virtual void FetchVertex(DrawContext *dc) = 0;
};

class VertexCachedFetcher: public VertexFetcher
{
public:
	VertexCachedFetcher();
	virtual ~VertexCachedFetcher() = default;

	virtual void emit(void *data);
	virtual void finalize();

protected:
	virtual void FetchVertex(DrawContext *dc);
};

} // namespace glsp
