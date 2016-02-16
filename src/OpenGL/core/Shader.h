#pragma once

#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include <glm/glm.hpp>

#include "NameSpace.h"
#include "PipeStage.h"
#include "DataFlow.h"
#include "Texture.h"
#include "compiler.h"


namespace glsp {


class GLContext;
class Shader;
struct Uniform;
struct VertexInfo;

typedef std::vector<Uniform> uniform_v;
typedef std::map<std::string, int> UniformMap;
typedef std::vector<VertexInfo> var_v;
typedef std::map<std::string, int> VarMap;


// GLSP extensions
GLAPI void* APIENTRY glspGetUniformLocation(GLuint program, const GLchar *name);

// NOTE:
// APP should use these two macros to define its own variables(name and type)
// for vertex shader varying: To make life easy, glPosition should come first!
#define DECLARE_IN(type, attr)	\
	m##attr = this->declareInput(#attr, typeid(type));	\
	m##attr <<= 2;

#define RESOLVE_IN(type, attr, input)	\
	type   &attr = reinterpret_cast<type &>(input.getReg(m##attr >> 2));

#define DECLARE_OUT(type, attr)	\
	m##attr = this->declareOutput(#attr, typeid(type));	\
	m##attr <<= 2;

#define RESOLVE_OUT(type, varying, output)	\
	type   &varying = reinterpret_cast<type &>(output.getReg(m##varying >> 2));


#define DECLARE_UNIFORM(uni)	\
	this->declareUniform(#uni, &uni);

#define DECLARE_SAMPLER(spl)	\
	this->declareSampler();		\
	this->declareUniform(#spl, &spl);

// NOTE:
// One shader should invoke this macro only once,
// i.e. there is only one texture coordinate in an vertex array.
#define DECLARE_TEXTURE_COORD(type, attr)	\
	this->SetTextureCoordLocation();			\
	DECLARE_IN(type, attr);


typedef unsigned int sampler1D;
typedef unsigned int sampler2D;


// APP need implement this interface
// and pass its pointer to glShaderSource.
class ShaderFactory
{
public:
	ShaderFactory():
		mVS(nullptr),
		mFS(nullptr)
	{}
	virtual ~ShaderFactory();

	virtual Shader *createVertexShader() = 0;
	virtual void DeleteVertexShader(Shader *pVS) = 0;

	virtual Shader *createFragmentShader() = 0;
	virtual void DeleteFragmentShader(Shader *pFS) = 0;

protected:
	Shader *mVS;
	Shader *mFS;
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

	int GetInRegLocation(const std::string &name);
	size_t getInRegsNum()  const { return mInRegs.size(); }

	int getSamplerNum() const { return mNumSamplers; }
	unsigned getSamplerUnitID(int i) const;

	bool HasSampler() const { return bHasSampler; }
	void SetupTextureInfo(unsigned unit, Texture *pTex)
	{
		assert(unit < MAX_TEXTURE_UNITS);
		assert(pTex != NULL);

		mTexs[unit] = pTex;
	}

	// NOTE: Can only be called in DECLARE_TEXTURE_COORD
	void SetTextureCoordLocation() { mTexCoordLoc = mInRegs.size(); }
	int  GetTextureCoordLocation() const { return mTexCoordLoc; }

	unsigned getOutRegsNum() const { return mOutRegs.size(); }

protected:
	template <class T>
	void declareUniform(const std::string &name, T *constant);

	int declareInput(const std::string &name, const std::type_info &type);
	int resolveInput(const std::string &name, const std::type_info &type);
	int declareOutput(const std::string &name, const std::type_info &type);
	int resolveOutput(const std::string &name, const std::type_info &type);

	void declareSampler();
	void setHasSampler()    { bHasSampler = true; }

	// Deprecated
	glm::vec4 texture2D(sampler2D sampler, const glm::vec2 &coord)
	{
		glm::vec4 res;

		//mTexs[sampler]->m_pfnTexture2D(this, mTexs[sampler], coord, res);

		return res;
	}

private:
	ShaderType mType;
	const char **mSource;
	uniform_v mUniformBlock;

	var_v mInRegs;
	VarMap mInRegsMap;

	var_v mOutRegs;
	VarMap mOutRegsMap;

	// Used to fast access the texture data in sampler2D().
	// TODO: only sampler2D() support so far
	Texture* mTexs[MAX_TEXTURE_UNITS];

	static const int kMaxSamplers = 4;
	bool		bHasSampler;
	int			mNumSamplers;
	unsigned	mSamplerLoc[kMaxSamplers];

	int 		mTexCoordLoc;
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
	void setVal(const T *val) const;

	template <class T>
	void getVal(T *val) const;

	void *const mPtr;
	const std::string mName;
	const std::type_info &mType;
};

template <class T>
Uniform::Uniform(T *val, const std::string &name):
	mPtr(val),
	mName(name),
	mType(typeid(T))
{
}

template <class T>
void Uniform::setVal(const T *val) const
{
	if((mType == typeid(T)) ||
		((mType == typeid(int) && typeid(T) == typeid(unsigned int)) ||
		(mType == typeid(unsigned int) && typeid(T) == typeid(int))))
	{
		*(static_cast<T *>(mPtr)) = *val;
	}
}

template <class T>
void Uniform::getVal(T *val) const
{
	if((mType == typeid(T)) ||
		((mType == typeid(int) && typeid(T) == typeid(unsigned int)) ||
		(mType == typeid(unsigned int) && typeid(T) == typeid(int))))
	{
		*val = *(static_cast<T *>(mPtr));
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

	bool getDiscardFlag() const { return bHasDiscard; }

protected:
	void texture2D(sampler2D sampler, Fsiosimd &fsio, __m128 &vS, __m128 &vT, __m128 vOut[]);

private:
	virtual void execute(fsInput& in, fsOutput& out);
	virtual void OnExecuteSIMD(Fsiosimd &fsio) { return; }

	void ExecuteSIMD(void *data);
	void ExecuteSISD(void *data);

	void SetDiscardFlag() { bHasDiscard = true; }

	bool bHasDiscard;
};

class Program: public NameItem
{
public:
	Program();
	virtual ~Program();

	VertexShader *getVS() const { return mVSLinked; }
	FragmentShader *getFS() const { return mFSLinked; }

	void AttachShader(Shader *pShader);
	void DetachShader(Shader *pShader);
	void LinkProgram();
	int  GetUniformLocation(const std::string &name);
	void* spGetUniformLocation(const std::string &name);

	template <class T>
	void UniformValue(int location, int count, const T *value);

private:
	VertexShader   *mVertexShader;
	FragmentShader *mFragmentShader;

	VertexShader   *mVSLinked;
	FragmentShader *mFSLinked;

	UniformMap mUniformMap;
	uniform_v mUniformBlock;
};

// FIXME: add support for arrays
template <class T>
void Program::UniformValue(int location, int count, const T *value)
{
	assert(count == 1);
	assert((size_t)location < mUniformBlock.size());

	Uniform & u = mUniformBlock[location];
	u.setVal(value);
}

class ProgramMachine
{
public:
	ProgramMachine();
	~ProgramMachine();

	unsigned CreateShader(GLContext *gc, unsigned type);
	void DeleteShader(GLContext *gc, unsigned shader);

	unsigned CreateProgram(GLContext *gc);
	void DeleteProgram(GLContext *gc, unsigned program);

	void ShaderSource(GLContext *gc, unsigned shader, int count, const char *const*string, const int *length);
	void CompileShader(GLContext *gc, unsigned shader);
	void AttachShader(GLContext *gc, unsigned program, unsigned shader);
	void DetachShader(GLContext *gc, unsigned program, unsigned shader);
	void LinkProgram(GLContext *gc, unsigned program);
	void UseProgram(GLContext *gc, unsigned program);
	unsigned char IsProgram(GLContext *gc, unsigned program);
	unsigned char IsShader(GLContext *gc, unsigned shader);

	int GetUniformLocation(GLContext *gc, unsigned program, const char *name);
	int GetAttribLocation(GLContext *gc, unsigned program, const char *name);
	void* spGetUniformLocation(GLContext *gc, unsigned program, const char *name);

	template <class T>
	void UniformMatrix(GLContext *gc, int location, int count, bool transpose, const T *value);

	template <class T>
	void UniformUif(GLContext *gc, int location, int count, const T *value);

	Program* getCurrentProgram() const { return mCurrentProgram; }

private:
	NameSpace mProgramNameSpace;
	NameSpace mShaderNameSpace;
	NameSpace mProgramPipelineNameSpace;
	Program  *mCurrentProgram;
};

// FIXME: add transpose support
template <class T>
void ProgramMachine::UniformMatrix(GLContext *gc, int location, int count, bool transpose, const T *value)
{
	GLSP_UNREFERENCED_PARAM(gc);

	assert(transpose == false);

	Program *pProg = getCurrentProgram();
	if(!pProg)
		return;

	pProg->UniformValue(location, count, value);
}

template <class T>
void ProgramMachine::UniformUif(GLContext *gc, int location, int count, const T *value)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = getCurrentProgram();
	if(!pProg)
		return;

	pProg->UniformValue(location, count, value);
}

} // namespace glsp
