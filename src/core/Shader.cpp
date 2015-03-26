#include <iostream>
#include "Shader.h"

using namespace std;

GLAPI GLuint APIENTRY glCreateShader (GLenum type)
{
	__GET_CONTEXT();
	return gc->mPM.CreateShader(gc, type);
}

GLAPI void APIENTRY glDeleteShader (GLuint shader)
{
	__GET_CONTEXT();
	gc->mPM.DeleteShader(gc, shader);
}

GLAPI GLuint APIENTRY glCreateProgram (void)
{
	__GET_CONTEXT();
	return gc->mPM.CreateProgram(gc);
}

GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length)
{
	__GET_CONTEXT();
	gc->mPM.ShaderSource(shader, count, string, length);
}

GLAPI void APIENTRY glCompileShader (GLuint shader)
{
	__GET_CONTEXT();
	gc->mPM.CompileShader(gc);
}

GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader)
{
	__GET_CONTEXT();
	gc->mPM.AttachShader(gc, program, shader);
}

// vertex shader cache
Shader::Shader(ShaderType eType)
{
	mType = eType;
	mSource = NULL;
}

void ShaderSource(char *src)
{
	mSource = src;
}

void VertexShader::compile()
{
	// No need to compile for embed shader
}

void VertexShader::execute()
{
	std::cout << __func__ << ": Please insert the code you want to excute!" << std::endl;
}

int VertexShader::SetVertexCount(unsigned int count)
{
	mVertexCount = count;
	return 0;
}

int VertexShader::SetAttribCount(unsigned int count)
{
	mAttribCount = count;
	return 0;
}

int VertexShader::SetVaryingCount(unsigned int count)
{
	mVaryingCount = count;
	return 0;
}

int VertexShader::AttribPointer(int index, void *ptr)
{
	mIn[index] = ptr;
	return 0;
}

int VertexShader::VaryingPointer(int index, void *ptr)
{
	mOut[index] = ptr;
	return 0;
}

void PixelShader::compile()
{
}

void PixelShader::attribPointer(float *attri)
{
	mIn = attri;
}

void PixelShader::setupOutputRegister(char *outReg)
{
	mOutReg = outReg;
}

// TODO: Inherit PixelShader to do concrete implementation
void PixelShader::execute()
{
	mOutReg[0] = (int)mIn[0];
	mOutReg[1] = (int)mIn[1];
	mOutReg[2] = (int)mIn[2];
	mOutReg[3] = (int)mIn[3];
}

class ShaderPlaceHolder: public NameItem
{
public:
	ShaderType mType;
};

unsigned ProgramMachine::CreateShader(GLContext *gc, unsigned type)
{
	unsigned name;
	GLSP_UNREFERENCED_PARAM(gc);

	if(mShaderNameSpace.genNames(1, &name))
	{
		ShaderPlaceHolder *pSPH = new ShaderPlaceHolder();
		pSPH->setName(name);
		pSPH->mType = Shader::OGLToInternal(type);
		mShaderNameSpace.insertObject(pSPH);
		return name;
	}
	else
	{
		return 0;
	}
}

void ProgramMachine::DeleteShader(GLContext *gc, unsigned shader)
{
	// TODO: impl
	GLSP_UNREFERENCED_PARAM(gc);
}

unsigned ProgramMachine::CreateProgram(GLContext *gc)
{
	unsigned name;
	GLSP_UNREFERENCED_PARAM(gc);

	if(mProgramNameSpace.genNames(1, name))
	{
		Program *pProg = new Program();
		pProg->setName(name);
		mProgramNameSpace.insertObject(pProg);
		return name;
	}
	else
	{
		return 0;
	}
}

void ProgramMachine::ShaderSource(
	GLContext *gc,
	unsigned shader,
	int count,
	const char *const*string,
	const int *length)
{
	GLSP_UNREFERENCED_PARAM(gc);
	GLSP_UNREFERENCED_PARAM(count);
	GLSP_UNREFERENCED_PARAM(length);

	Shader *pShader;
	ShaderFactory *pFact = reinterpret_cast<ShaderFactory *>(string);
	ShaderPlaceHolder *pSPH = mShaderNameSpace.retrieveObject(shader);

	if(pSPH->mType == Shader::VERTEX)
		pShader = pFact->createVertexShader();
	else if(pSPH->mType == Shader::FRAGMENT)
		pShader = pFact->createFragmentShader();

	if(!pShader)
		return;
	
	mShaderNameSpace.removeObject(pSPH);
	pShader->setName(shader);
	pShader->shaderSource(string);
	mShaderNameSpace.insertObject(pShader);
}

void ProgramMachine::CompileShader(GLContext *gc)
{
	GLSP_UNREFERENCED_PARAM(gc);
	return;
}

void ProgramMachine::AttachShader(GLContext *gc, program, shader)
{
	Program *pProg = mProgramNameSpace.retrieveObject(program);
	Shader *pShader = mShaderNameSpace.retrieveObject(shader);

	if(!pProg || !pShader)
		return;
	
	pProg->attachShader(pShader);
}
