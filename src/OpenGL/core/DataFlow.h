#pragma once

#include <glm/glm.hpp>

#include <common/glsp_defs.h>
#include <list>
#include <vector>
#include <utility>


NS_OPEN_GLSP_OGL()

struct DrawContext;

class ShaderRegisterFile
{
public:
	typedef std::vector<glm::vec4> RegArray;

	ShaderRegisterFile() = default;
	ShaderRegisterFile(const ShaderRegisterFile&) = default;
	ShaderRegisterFile& operator=(const ShaderRegisterFile&) = default;
	~ShaderRegisterFile() = default;

	// Move semantics
	ShaderRegisterFile(ShaderRegisterFile &&rhs)
	{
		mRegs.swap(rhs.getRegArray());
	}
	ShaderRegisterFile& operator=(ShaderRegisterFile &&rhs)
	{
		mRegs.swap(rhs.mRegs);
		return *this;
	}
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

	RegArray& getRegArray()
	{
		return mRegs;
	}
	const RegArray& getRegArray() const
	{
		return mRegs;
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

	size_t size() const
	{
		return mRegs.size();
	}

	glm::vec4& operator[](int idx)
	{
		return getReg(idx);
	}
	const glm::vec4& operator[](int idx) const
	{
		return getReg(idx);
	}

	ShaderRegisterFile& operator+=(const ShaderRegisterFile &rhs);
	ShaderRegisterFile& operator*=(const float scalar);

private:
	RegArray mRegs;
};

ShaderRegisterFile operator+(const ShaderRegisterFile &lhs, const ShaderRegisterFile &rhs);
ShaderRegisterFile operator*(const ShaderRegisterFile &v, float scalar);
ShaderRegisterFile operator*(float scalar, const ShaderRegisterFile &v);


typedef ShaderRegisterFile vsInput;
typedef ShaderRegisterFile vsOutput;

typedef ShaderRegisterFile fsInput;
typedef ShaderRegisterFile fsOutput;


// TODO: add operator =
struct Primitive
{
	Primitive() = default;
	~Primitive() = default;

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

	// The reciprocal of the directed area of a triangle.
	// FIXME: primitive may be not a triangle.
	float mAreaReciprocal;
};

typedef std::vector<int> IBuffer_v;
typedef std::vector<vsInput> vsInput_v;
typedef std::list<Primitive> Primlist;
typedef std::vector<vsOutput> vsOutput_v;


/* TODO: comment
 * Batch represents a batch of data flow to be passed through the whole pipeline
 * It's hard to give a decent name to each member based on their repective usages.
 * Here is rough explanation:
 * Vertex Fetch: read data from VBO, produce mVertexCache & mIndexBuf.
 * Vertex Shading: consumer mVertexCache, produce mVsOut
 * Primitive Assembly: consumer mIndexBuf & mVsOut, produce mPrims
 * Clipping ~ Viewport transform: consumer mPrims, produce mPrims
 */
class Batch
{
public:
	Batch() = default;
	~Batch() = default;

	Batch& splice(Batch &rhs)
	{
		mPrims.splice(mPrims.end(), rhs.mPrims);
		return *this;
	}

	// Move semantics
	Batch& operator+=(Batch &&rhs)
	{
		mPrims.splice(mPrims.end(), rhs.mPrims);
		return *this;
	}

	vsInput_v		mVertexCache;
	vsOutput_v		mVsOut;
	IBuffer_v		mIndexBuf;
	Primlist		mPrims;

	// point back to the mDC
	DrawContext	   *mDC;
};

class Gradience
{
public:
	Gradience(const Primitive &prim):
		mPrim(prim)
	{
	}
	~Gradience() = default;

	fsInput	mStarts[Primitive::MAX_PRIM_TYPE];

	// X/Y partial derivatives
	fsInput	mGradiencesX;
	fsInput	mGradiencesY;

	const Primitive	&mPrim;

#if 1
	// used to compute lambda. Refer to:
	// http://www.gamasutra.com/view/feature/3301/runtime_mipmap_filtering.php?print=1
	float a, b, c, d, e, f;
#endif
};

class Fsio
{
public:
	Fsio() = default;
	~Fsio() = default;

	fsInput in;
	fsOutput out;

	const Gradience *mpGrad;

	int 	x, y;
	float 	z;
	int 	mIndex; // used to lookup the color/depth/stencil buffers

	// used to indicate if in is already interpolated or not
	// FIXME: looks wired, remove this flag
	bool bValid;

	void *m_priv;
};

NS_CLOSE_GLSP_OGL()
