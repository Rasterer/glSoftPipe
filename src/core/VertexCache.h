#pragma once

class vsInput;

typedef std::map<vsInput, int> vsCache;
typedef std::vector<int> vsIndex;

struct Batch
{
	vsCache mCache;
	vsIndex mIndex;
};

class VertexCachedAssembler: public stage
{
public:
	VertexCachedAssembler();
	void assemble();
	void dispatch(Batch &batch);
};