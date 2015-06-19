#include "DataFlow.h"
#include "DrawEngine.h"


NS_OPEN_GLSP_OGL()

ShaderRegisterFile& ShaderRegisterFile::operator+=(const ShaderRegisterFile &rhs)
{
	assert(getRegsNum() == rhs.getRegsNum());

	for(size_t i = 0; i < getRegsNum(); i++)
	{
		mRegs[i] += rhs[i];
	}
	return *this;
}

ShaderRegisterFile& ShaderRegisterFile::operator*=(const float scalar)
{
	for(size_t i = 0; i < getRegsNum(); i++)
	{
		mRegs[i] *= scalar;
	}
	return *this;
}

ShaderRegisterFile operator+(const ShaderRegisterFile &lhs, const ShaderRegisterFile &rhs)
{
	assert(lhs.size() == rhs.size());

	ShaderRegisterFile tmp;
	tmp.resize(lhs.size());

	for(size_t i = 0; i < lhs.size(); i++)
	{
		tmp[i] = lhs[i] + rhs[i];
	}

	return std::move(tmp);
}

ShaderRegisterFile operator*(const ShaderRegisterFile &v, float scalar)
{
	ShaderRegisterFile tmp;
	tmp.resize(v.size());

	for(size_t i = 0; i < v.size(); i++)
	{
		tmp[i] = v[i] * scalar;
	}

	return std::move(tmp);
}

ShaderRegisterFile operator*(float scalar, const ShaderRegisterFile &v)
{
	ShaderRegisterFile tmp;
	tmp.resize(v.size());

	for(size_t i = 0; i < v.size(); i++)
	{
		tmp[i] = v[i] * scalar;
	}

	return std::move(tmp);
}


NS_CLOSE_GLSP_OGL()
