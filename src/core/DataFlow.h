#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <list>
#include <unordered_map>

typedef std::vector<glm::vec4> RegArray;

class vertex_data
{
public:
	// getReg() and resize is one pair
	void resize(size_t n)
	{
		mRegs.resize(n);
	}
	glm::vec4 & getReg(int location)
	{
		assert(location < (int)mRegs.size());
		return mRegs[location];
	}

	glm::vec4 & position()
	{
		return mRegs[0];
	}

	// reserve() and assemble() is one pair
	void reserve(size_t n)
	{
		mRegs.reserve(n);
	}

	void assemble(const glm::vec4 &attr)
	{
		mRegs.push_back(attr);
	}

	size_t getRegsNum() const
	{
		return mRegs.size();
	}

private:
	RegArray mRegs;
};

typedef vertex_data vsInput;
typedef vertex_data vsOutput;

struct Primitive
{
	enum PrimType
	{
		POINT = 0,
		LINE,
		TRIANGLE
	};
	
	PrimType mType;

#if PRIMITIVE_REFS_VERTICES
	vsOutput *mVert[3];
#elif PRIMITIVE_OWNS_VERTICES
	vsOutput mVert[3];
#endif
};

typedef std::vector<int> IBuffer_v;
typedef std::unordered_map<int, int> vsCacheIndex;
typedef std::vector<vsInput> vsCache;
typedef std::list<Primitive> PrimBatch;

#if PRIMITIVE_REFS_VERTICES
typedef std::vector<vsOutput *> vsOutput_v;
#elif PRIMITIVE_OWNS_VERTICES
typedef std::vector<vsOutput> vsOutput_v;
#endif

// TODO: comment
// Batch represents a batch of data flow to be passed through the whole pipeline
// It's hard to give a decent name to each member based on their repective usages.
// Here is rough explanation:
// Input Assembly: read data from VBO, produce mVertexCache & mIndexBuf.
// Vertex Shading: consumer mVertexCache, produce mVsOut
// Primitive Assembly: consumer mIndexBuf & mVsOut, produce mPrim
// Clipping ~ Viewport transform: consumer mPrim, produce mPrim
struct Batch
{
	vsCache			mVertexCache;
	vsCacheIndex	mCacheIndex;
	vsOutput_v		mVsOut;
	IBuffer_v		mIndexBuf;
	PrimBatch		mPrim;
};
