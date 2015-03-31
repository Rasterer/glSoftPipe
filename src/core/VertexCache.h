#pragma once

#include "Shader.h"
#include "PipeStage.h"

#define VERTEX_CACHE_EMIT_THRESHHOLD 36

typedef std::vector<int> vsIndex;
typedef std::vector<vsOutput> vsOutBuf;

struct vsInputHash
{
	size_t operator () (const vsInput &in) const 
	{
		size_t ret = 0;
		unsigned char *pos = reinterpret_cast<unsigned char *>(const_cast<vsInput &>(in).getAttrib(0));

		for(size_t i = 0; i < 16; i++)
			ret += pos[i];

		return ret;
	}
};

struct vsInputComp
{
	bool operator () (const vsInput &lhs, const vsInput &rhs)
	{
		vsInput &a = const_cast<vsInput &>(lhs);
		vsInput &b = const_cast<vsInput &>(rhs);

		return a == b;
	}
};

typedef std::unordered_map<vsInput, int, vsInputHash, vsInputComp> vsCache;

struct Batch
{
	vsCache	mCache;
	vsOutBuf mOut;
	vsIndex mIndex;
};

class VertexCachedAssembler: public PipeStage
{
public:
	VertexCachedAssembler();
	virtual void emit(void *data);
	void assembleVertex();
	void dispatch(Batch &batch);
};
