#pragma once

#include <typeinfo>
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "glsp_defs.h"
#include "NameSpace.h"

using namespace std;
using namespace glm;

class NameSpace;
class GLContext;
class uniform;
class Shader;
class PerVertVar;

typedef vector<uniform> vUniform_t;
typedef map<string, int> UniformMap;
typedef vector<PerVertVar> var_t;
typedef map<string, int> VarMap;
typedef vector<vec4> RegArray;

// For vertex shader:
// APP should use these two macros to define its own attributes(name and type)
#define VS_DECLARE_ATTRIB(type, attr)	\
	this->declareAttrib(#attr, typeid(type));

#define VS_RESOLVE_ATTRIB(type, attr, input)	\
	int a_##attr = this->resolveAttrib(#attr, typeid(type));	\
	type &attr = *(reinterpret_cast<type *>(input.getAttrib(a_##attr)));

#define VS_DECLARE_VARYING(type, attr)	\
	this->declareVarying(#attr, typeid(type));

#define VS_RESOLVE_VARYING(type, varying, output)	\
	int a_##varying = this->resolveVarying(#varying, typeid(type));	\
	type &varying = *(reinterpret_cast<type *>(output.getVarying(a_##varying)));

#define DECLARE_UNIFORM(uni)	\
	this->declareUniform(#uni, &uni);

// APP need implement this interface
// and pass its pointer to glShaderSource.
class ShaderFactory
{
public:
	virtual Shader *createVertexShader() = 0;
	virtual void DeleteVertexShader(Shader *pVS) = 0;
	virtual Shader *createFragmentShader() = 0;
	virtual void DeleteFragmentShader(Shader *pFS) = 0;
};

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
	static ShaderType OGLToInternal(unsigned type);

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

	inline vUniform_t & getUniformBlock()
	{
		return mUniformRegs;
	}

	template <class T>
	void declareUniform(const string &name, T *constant)
	{
		mUniformRegs.push_back(uniform(constant, name));
	}

	virtual void compile() = 0;
	virtual void execute() = 0;

private:
	ShaderType mType;
	const char **mSource;
	vUniform_t mUniformRegs;
};

// Per vertex variable: attribute or varying
struct PerVertVar
{
	PerVertVar(const string &name, const type_info &type):
		mName(name),
		mType(type)
	{
	}
	const string mName;
	const type_info &mType;
};

struct uniform
{
	template <class T>
	uniform(T *val, const string &name):
		mPtr(static_cast<void *>(val)),
		mName(name),
		mType(typeid(T))
	{
	}

	template <class T>
	void setVal(const T *val)
	{
		if(mType == typeid(T))
		{
			*(static_cast<T *>(mPtr)) = *val;
		}
	}

	void *mPtr;
	const string mName;
	const type_info &mType;
};

class vsInput
{
public:
	inline vec4 *getAttrib(int location)
	{
		assert(location < (int)mRegs.size());
		return &(mRegs[location]);
	}

	inline void assemble(const vec4 &attr)
	{
		mRegs.push_back(attr);
	}

private:
	RegArray mRegs;
};

class vsOutput
{
public:
	inline vec4 *getVarying(int location)
	{
		assert(location < (int)mRegs.size());
		return &(mRegs[location]);
	}

	inline void outputSize(int n)
	{
		mRegs.resize(n);
	}

private:
	RegArray mRegs;
};

class VertexShader: public Shader
{
public:
	VertexShader() {}
	virtual void compile();
	virtual void execute();
	virtual void onExecute(vsInput &in, vsOutput &out);
	void declareAttrib(const string &name, const type_info &type);
	int resolveAttrib(const string &name, const type_info &type);
	void declareVarying(const string &name, const type_info &type);
	int resolveVarying(const string &name, const type_info &type);
	int GetAttribLocation(const string &name);

private:
	var_t mAttribRegs;
	VarMap mAttribMap;

	var_t mVaryingRegs;
	VarMap mVaryingMap;
};

class FragmentShader: public Shader
{
public:
	FragmentShader() {}
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

	inline VertexShader *getVS()
	{
		return mVertexShader;
	}
	inline FragmentShader *getFS()
	{
		return mFragmentShader;
	}
	inline void attachShader(Shader *pShader)
	{
		if(pShader->getType() == Shader::VERTEX)
			mVertexShader = static_cast<VertexShader *>(pShader);
		else if(pShader->getType() == Shader::FRAGMENT)
			mFragmentShader = static_cast<FragmentShader *>(pShader);
	}

	inline bool validate()
	{
		if(mVertexShader && mFragmentShader)
			return true;
		else
			return false;
	}

	void LinkProgram();
	int GetUniformLocation(const string &name);

	template <class T>
	void UniformValue(int location, int count, bool transpose, const T *value)
	{
		assert(count == 1);
		assert(transpose == false);
		assert(location < mUniformRegs.size());

		uniform & u = mUniformRegs[location];
		u.setVal(value);
	}

private:
	VertexShader *mVertexShader;
	FragmentShader *mFragmentShader;

	UniformMap mUniformMap;
	vUniform_t mUniformRegs;
};

class ProgramMachine
{
public:
	ProgramMachine();
	unsigned CreateShader(GLContext *gc, unsigned type);
	void DeleteShader(GLContext *gc, unsigned shader);
	unsigned CreateProgram(GLContext *gc);
	void DeleteProgram(GLContext *gc, unsigned program);
	void ShaderSource(GLContext *gc, unsigned shader, int count, const char *const*string, const int *length);
	void CompileShader(GLContext *gc, unsigned shader);
	void AttachShader(GLContext *gc, unsigned program, unsigned shader);
	void LinkProgram(GLContext *gc, unsigned program);
	void UseProgram(GLContext *gc, unsigned program);
	int  GetUniformLocation(GLContext *gc, unsigned program, const char *name);
	int  GetAttribLocation(GLContext *gc, unsigned program, const char *name);

	template <class T>
	void UniformValue(GLContext *gc, int location, int count, bool transpose, const T *value)
	{
		GLSP_UNREFERENCED_PARAM(gc);

		Program *pProg = mCurrentProgram;
		if(!pProg)
			return;

		pProg->UniformValue(location, count, transpose, value);
	}

private:
	NameSpace mProgramNameSpace;
	NameSpace mShaderNameSpace;
	NameSpace mProgramPipelineNameSpace;
	Program *mCurrentProgram;
};
