#pragma once

#include <typeinfo>
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "glcorearb.h"
#include "NameSpace.h"
#include "VertexArrayObject.h"
#include "shader_export.h"

using namespace std;
using namespace glm;

#define MAX_ATTRIBUTE_NUM 16

class GLContext;
class uniform;

// TODO: use shader compiler
class Shader: public NameItem
{
public:
	enum ShaderType
	{
		INVALID,
		VERTEX,
		FRAGMENT
	};

	Shader();
	static ShaderType OGLToInternal(unsigned type)
	{
		switch(type)
		{
			case GL_VERTEX_SHADER:		return VERTEX;
			case GL_FRAGMENT_SHADER:	return FRAGMENT;
			default:					return INVALID;
		}
	}

	inline void shaderSource(const char **string)
	{
		mSource = string;
	}
	inline void setType(ShaderType eType)
	{
		mType = eType;
	}
	inline ShaderType getType()
	{
		return mType;
	}

	template <class T>
	void declareUniform(string &name, T &constant)
	{
		mUniformMap[name] = mUniformRegs.size();
		mUniformRegs.push_back(uniform(constant));
	}

	int GetUniformLocation(string &name)
	{
		UniformMap::iterator it = mUniformMap.find(name);

		if(it != mUniformMap.end())
			return it->second;
		else
			return -1;
	}

	virtual void compile() = 0;
	virtual void execute() = 0;

private:
	typedef map<string, int> UniformMap;
	typedef vector<uniform> vUniform_t;

	ShaderType mType;
	const char **mSource;
	UniformMap mUniformMap;
	vUniform_t mUniformRegs;
};

typedef vector<vec4> RegArray;

class vsInput
{
public:
	void declareAttrib(string name, size_t size);
	void defineAttrib(string name, size_t size);
private:	
	// Attrib name and location mapping.
	typedef map<string, int> vsAttribMap;

	vsAttribMap mNamedLocation;
	RegArray mInReg;
};

class vsOutput
{
public:
	void declareVarying(string name, size_t size);
	void defineVarying(string name, size_t size);
private:	
	RegArray mOutReg;
};

class VertexShader: public Shader
{
public:
	virtual void compile();
	virtual void execute(vsInput, vsOutput);
	VertexShader() {}

private:

};

class PixelShader: public Shader
{
public:
	PixelShader() {}
	virtual void compile();
	virtual void execute();
	virtual void attribPointer(float *attri);
	virtual void setupOutputRegister(char *outReg);
	
	float *mIn;
	char *mOutReg;
};

class Program: public NameItem
{
public:
	Program(): mVertexShader(NULL), mFragmentShader(NULL) {}

	inline Shader *getVS()
	{
		return mVertexShader;
	}
	inline Shader *getFS()
	{
		return mFragmentShader;
	}
	void attachShader(Shader *pShader)
	{
		if(pShader->getType() == Shader::VERTEX)
			mVertexShader = pShader;
		else if(pShader->getType() == Shader::FRAGMENT)
			mFragmentShader = pShader;
	}
	inline bool validate()
	{
		if(mVertexShader && mFragmentShader)
			return true;
		else
			return false;
	}

private:
	Shader *mVertexShader;
	Shader *mFragmentShader;
};

class ProgramMachine
{
public:
	unsigned CreateShader(GLContext *gc, unsigned type);
	void DeleteShader(GLContext *gc, unsigned shader);
	unsigned CreateProgram(GLContext *gc);
	void DeleteProgram(GLContext *gc, unsigned program);
	void ShaderSource(GLContext *gc, unsigned shader, int count, const char *const*string, const int *length);
	void CompileShader(GLContext *gc, unsigned shader);
	void AttachShader(GLContext *gc, unsigned program, unsigned shader);
	void UseProgram(GLContext *gc, unsigned program);
	int  GetUniformLocation(GLContext, unsigned program, const char *name);
	void UniformValue(GLContext *gc, int location, int count, bool transpose, const float *value);

private:
	NameSpace mProgramNameSpace;
	NameSpace mShaderNameSpace;
	NameSpace mProgramPipelineNameSpace;
	Program *mCurrentProgram;
};

class uniform
{
	template <class T>
	uniform(T val):
		mPtr(static_cast<void *>(&val)),
		mType(typeid(val))
	{
	}
private:
	void *mPtr;
	type_info &mType;
};
