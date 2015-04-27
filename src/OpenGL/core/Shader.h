#pragma once

#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include <glm/glm.hpp>

#include "common/glsp_defs.h"
#include "NameSpace.h"
#include "PipeStage.h"
#include "DataFlow.h"


NS_OPEN_GLSP_OGL()

class GLContext;
class Uniform;
class Shader;
class VertexInfo;

typedef std::vector<Uniform> uniform_v;
typedef std::map<std::string, int> UniformMap;
typedef std::vector<VertexInfo> var_v;
typedef std::map<std::string, int> VarMap;

// APP should use these two macros to define its own variables(name and type)
// for vertex shader varying: To make life easy, glPosition should come first!
#define DECLARE_IN(type, attr)	\
	this->declareInput(#attr, typeid(type));

#define RESOLVE_IN(type, attr, input)	\
	int l_##attr = this->resolveInput(#attr, typeid(type));	\
	type &attr = reinterpret_cast<type &>(input.getReg(l_##attr));

#define DECLARE_OUT(type, attr)	\
	this->declareOutput(#attr, typeid(type));

#define RESOLVE_OUT(type, varying, output)	\
	int l_##varying = this->resolveOutput(#varying, typeid(type));	\
	type &varying = reinterpret_cast<type &>(output.getReg(l_##varying));

#define DECLARE_UNIFORM(uni)	\
	this->declareUniform(#uni, &uni);

// APP need implement this interface
// and pass its pointer to glShaderSource.
class ShaderFactory
{
public:
	ShaderFactory() { }
	virtual ~ShaderFactory() {};

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
	virtual ~Shader() { }

	virtual void compile() = 0;

	static ShaderType OGLShaderTypeToInternal(unsigned type);

	// mutators
	void setSource(const char **string) { mSource = string; }
	void setType(ShaderType eType) { mType = eType; }

	// accessors
	ShaderType getType() const { return mType; }
	uniform_v & getUniformBlock() { return mUniformBlock; }
	const char **getSource() const { return mSource; }

	template <class T>
	void declareUniform(const std::string &name, T *constant);

	void declareInput(const std::string &name, const std::type_info &type);
	int resolveInput(const std::string &name, const std::type_info &type);
	void declareOutput(const std::string &name, const std::type_info &type);
	int resolveOutput(const std::string &name, const std::type_info &type);

	int GetInRegLocation(const std::string &name);
	size_t getInRegsNum()  const { return mInRegs.size(); }
	size_t getOutRegsNum() const { return mOutRegs.size(); }

private:
	ShaderType mType;
	const char **mSource;
	uniform_v mUniformBlock;

	var_v mInRegs;
	VarMap mInRegsMap;

	var_v mOutRegs;
	VarMap mOutRegsMap;
};

template <class T>
void Shader::declareUniform(const std::string &name, T *constant)
{
	mUniformBlock.push_back(Uniform(constant, name));
}

// Per vertex variable: attribute or varying
struct VertexInfo
{
	VertexInfo(const std::string &name, const std::type_info &type);
	~VertexInfo() { }

	const std::string mName;
	const std::type_info &mType;
};

struct Uniform
{
	template <class T>
	Uniform(T *val, const std::string &name);
	~Uniform() { }

	template <class T>
	void setVal(const T *val);

	void *mPtr;
	const std::string mName;
	const std::type_info &mType;
};

template <class T>
Uniform::Uniform(T *val, const std::string &name):
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
	virtual ~VertexShader() { }

	virtual void emit(void *data);
	virtual void finalize();

	virtual void compile();

protected:
	// App should rewrite this method
	virtual void execute(vsInput &in, vsOutput &out);

private:
};

class FragmentShader: public Shader,
					  public PipeStage
{
public:
	FragmentShader();
	virtual ~FragmentShader() { }

	virtual void emit(void *data);
	virtual void finalize();
	virtual void compile();

private:
	virtual void execute(fsInput& in, fsOutput& out);

};

class Program: public NameItem
{
public:
	Program();
	virtual ~Program() { }

	VertexShader *getVS() const { return mVertexShader; }
	FragmentShader *getFS() const { return mFragmentShader; }

	void AttachShader(Shader *pShader);
	void LinkProgram();
	int  GetUniformLocation(const std::string &name);

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
	~ProgramMachine() { }

	unsigned CreateShader(GLContext *gc, unsigned type);
	void DeleteShader(GLContext *gc, unsigned shader);

	unsigned CreateProgram(GLContext *gc);
	void DeleteProgram(GLContext *gc, unsigned program);

	void ShaderSource(GLContext *gc, unsigned shader, int count, const char *const*string, const int *length);
	void CompileShader(GLContext *gc, unsigned shader);
	void AttachShader(GLContext *gc, unsigned program, unsigned shader);
	void LinkProgram(GLContext *gc, unsigned program);
	void UseProgram(GLContext *gc, unsigned program);

	int GetUniformLocation(GLContext *gc, unsigned program, const char *name);
	int GetAttribLocation(GLContext *gc, unsigned program, const char *name);

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

NS_CLOSE_GLSP_OGL()