#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <list>


NS_OPEN_GLSP_OGL()

class ShaderRegisterFile
{
public:
	// getReg() and resize() is one pair
	void resize(size_t n)
	{
		mRegs.resize(n);
	}
	glm::vec4& getReg(int location)
	{
		assert(location < (int)mRegs.size());
		return mRegs[location];
	}
	const glm::vec4& getReg(int location) const
	{
		assert(location < (int)mRegs.size());
		return mRegs[location];
	}

	// glPosition and FragColor are both in the first location,
	// which can be treat as a union.
	glm::vec4& position()
	{
		return mRegs[0];
	}
	glm::vec4& fragcolor()
	{
		return mRegs[0];
	}

	const glm::vec4& position() const
	{
		return mRegs[0];
	}
	const glm::vec4& fragcolor() const
	{
		return mRegs[0];
	}

	// reserve() and pushReg() is one pair
	void reserve(size_t n)
	{
		mRegs.reserve(n);
	}

	void pushReg(const glm::vec4 &attr)
	{
		mRegs.push_back(attr);
	}

	size_t getRegsNum() const
	{
		return mRegs.size();
	}

private:
	typedef std::vector<glm::vec4> RegArray;
	RegArray mRegs;
};

typedef ShaderRegisterFile vsInput;
typedef ShaderRegisterFile vsOutput;

typedef ShaderRegisterFile fsInput;
typedef ShaderRegisterFile fsOutput;

struct Primitive
{
	enum PrimType
	{
		POINT = 0,
		LINE,
		TRIANGLE,
		MAX_PRIM_TYPE
	};
	
	PrimType mType;

	int mVertNum;
	vsOutput mVert[MAX_PRIM_TYPE];

	float mAreaReciprocal;

	std::vector<glm::vec4> mGradiences;
};

typedef std::vector<int> IBuffer_v;
typedef std::vector<vsInput> vsInput_v;
typedef std::list<Primitive> Primlist;
typedef std::vector<vsOutput> vsOutput_v;


// TODO: comment
// Batch represents a batch of data flow to be passed through the whole pipeline
// It's hard to give a decent name to each member based on their repective usages.
// Here is rough explanation:
// Vertex Fetch: read data from VBO, produce mVertexCache & mIndexBuf.
// Vertex Shading: consumer mVertexCache, produce mVsOut
// Primitive Assembly: consumer mIndexBuf & mVsOut, produce mPrims
// Clipping ~ Viewport transform: consumer mPrims, produce mPrims
class Batch
{
public:
	void MergePrim(Batch &rhs)
	{
		mPrims.splice(mPrims.end(), rhs.mPrims)
	}

	vsInput_v		mVertexCache;
	vsOutput_v		mVsOut;
	IBuffer_v		mIndexBuf;
	Primlist		mPrims;

	// point back to the mDC
	DrawContext	   *mDC;
};

NS_CLOSE_GLSP_OGL()
