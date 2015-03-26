#pragma once

#include <string>
#include <glm/glm.hpp>
#include "glcorearb.h"
#include "NameSpace.h"
#define "VertexArrayObject.h"

#define MAX_ATTRIBUTE_NUM 16

#define VS_DECLARE_ATTRIB(attr, type)	\
	type &attr = this->getAttrib(#attr, sizeof(type));

#define VS_DEFINE_ATTRIB(attr, type)	\
	type &attr = this->getAttrib(#attr, sizeof(type));

class GLContext;

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
	virtual void compile() = 0;
	virtual void execute() = 0;

private:
	ShaderType mType;
	const char **mSource;
};

typedef vector<vec4> RegArray;

class vsInput
{
public:
private:	
	RegArray mInReg;
};

class vsOutput
{
public:
private:	
	RegArray mOnReg;
};

class VertexShader: public Shader
{
public:
	virtual void compile();
	virtual void execute(vsInput, vsOutput);
	VertexShader(): Shader(VERTEX) {}
	declareAttrib(string name, size_t size);
	defineAttrib(string name, size_t size);

private:
	typedef map<string, int> vsAttribMap;

	// Attrib name and location mapping.
	vsAttribMap mNamedLocation;
};

class PixelShader: public Shader
{
public:
	PixelShader(): Shader(PIXEL) {}
	virtual void compile();
	virtual void execute();
	virtual void attribPointer(float *attri);
	virtual void setupOutputRegister(char *outReg);
	
	float *mIn;
	char *mOutReg;
};

class ShaderFactory
{
public:
	virtual Shader *createVertexShader() = 0;
	virtual Shader *createFragmentShader() = 0;
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
	void ShaderSource(GLContext *gc, unsigned shader, int count, const char *const*string, const int *length);
	void CompileShader(GLContext *gc);
	void AttachShader(GLContext *gc, program, shader);

private:
	NameSpace mProgramNameSpace;
	NameSpace mShaderNameSpace;
	NameSpace mProgramPipelineNameSpace;
	Program *mCurrentProgram;
};
