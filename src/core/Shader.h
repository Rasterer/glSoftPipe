#pragma once

#include <typeinfo>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "glsp_defs.h"
#include "NameSpace.h"
#include "PipeStage.h"
#include "DataFlow.h"

class GLContext;
class Uniform;
class Shader;
class VertexInfo;

typedef std::vector<Uniform> uniform_v;
typedef std::map<string, int> UniformMap;
typedef std::vector<VertexInfo> var_v;
typedef std::map<string, int> VarMap;

// For vertex shader:
// APP should use these two macros to define its own attributes(name and type)
// for varying: To make life easy, glPosition should come first!
#define VS_DECLARE_ATTRIB(type, attr)	\
	this->declareAttrib(#attr, typeid(type));

#define VS_RESOLVE_ATTRIB(type, attr, input)	\
	int a_##attr = this->resolveAttrib(#attr, typeid(type));	\
	type &attr = reinterpret_cast<type &>(input.getReg(a_##attr));

#define VS_DECLARE_VARYING(type, attr)	\
	this->declareVarying(#attr, typeid(type));

#define VS_RESOLVE_VARYING(type, varying, output)	\
	int a_##varying = this->resolveVarying(#varying, typeid(type));	\
	type &varying = reinterpret_cast<type &>(output.getReg(a_##varying));

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

	// mutators
	void setSource(const char **string) { mSource = string; }
	void setType(ShaderType eType) { mType = eType; }

	// accessors
	ShaderType getType() const { return mType; }
	uniform_v & getUniformBlock() { return mUniformBlock; }
	const char **getSource() const { return mSource; }

	template <class T>
	void declareUniform(const string &name, T *constant);

private:
	ShaderType mType;
	const char **mSource;
	uniform_v mUniformBlock;
};

template <class T>
void Shader::declareUniform(const string &name, T *constant)
{
	mUniformBlock.push_back(Uniform(constant, name));
}

// Per vertex variable: attribute or varying
struct VertexInfo
{
	VertexInfo(const string &name, const type_info &type);

	const string mName;
	const type_info &mType;
};

struct Uniform
{
	template <class T>
	Uniform(T *val, const string &name);

	template <class T>
	void setVal(const T *val);

	void *mPtr;
	const string mName;
	const type_info &mType;
};

template <class T>
Uniform::Uniform(T *val, const string &name):
	mPtr(static_cast<void *>(val)),
	mName(name),
	mType(typeid(T))
{
}

template <class T>
void Uniform::setVal(const T *val)
{
	if(mType == typeid(T))
	{
		*(static_cast<T *>(mPtr)) = *val;
	}
}

class VertexShader: public Shader,
					public PipeStage
{
public:
	VertexShader();
	virtual void emit(void *data);
	virtual void compile();
	void declareAttrib(const string &name, const type_info &type);
	int resolveAttrib(const string &name, const type_info &type);
	void declareVarying(const string &name, const type_info &type);
	int resolveVarying(const string &name, const type_info &type);
	int GetAttribLocation(const string &name);
	size_t getAttribNum() const { return mAttribRegs.size(); }
	size_t getVaryingNum() const { return mVaryingRegs.size(); }

private:
	virtual void execute(vsInput &in, vsOutput &out);

private:
	var_v mAttribRegs;
	VarMap mAttribMap;

	var_v mVaryingRegs;
	VarMap mVaryingMap;
};

// TODO: rework
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
	Program();

	VertexShader *getVS() const { return mVertexShader; }
	FragmentShader *getFS() const { return mFragmentShader; }

	void attachShader(Shader *pShader);
	void LinkProgram();
	int GetUniformLocation(const string &name);

	template <class T>
	void UniformValue(int location, int count, bool transpose, const T *value);

private:
	bool validate();

private:
	VertexShader *mVertexShader;
	FragmentShader *mFragmentShader;

	UniformMap mUniformMap;
	uniform_v mUniformBlock;
};

template <class T>
void Program::UniformValue(int location, int count, bool transpose, const T *value)
{
	assert(count == 1);
	assert(transpose == false);
	assert(location < mUniformBlock.size());

	Uniform & u = mUniformBlock[location];
	u.setVal(value);
}

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
	void UniformValue(GLContext *gc, int location, int count, bool transpose, const T *value);

	Program *getCurrentProgram() const { return mCurrentProgram; }

private:
	void setCurrentProgram(Program *prog) { mCurrentProgram = prog; }

private:
	NameSpace mProgramNameSpace;
	NameSpace mShaderNameSpace;
	NameSpace mProgramPipelineNameSpace;
	Program *mCurrentProgram;
};

template <class T>
void ProgramMachine::UniformValue(GLContext *gc, int location, int count, bool transpose, const T *value)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = getCurrentProgram();
	if(!pProg)
		return;

	pProg->UniformValue(location, count, transpose, value);
}
