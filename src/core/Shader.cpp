#include <iostream>
#include "Shader.h"
#include "GLContext.h"
#include "glsp_defs.h"

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

GLAPI void APIENTRY glDeleteProgram (GLuint program)
{
	__GET_CONTEXT();
	gc->mPM.DeleteProgram(gc, program);
}

GLAPI void APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length)
{
	__GET_CONTEXT();
	gc->mPM.ShaderSource(gc, shader, count, string, length);
}

GLAPI void APIENTRY glCompileShader (GLuint shader)
{
	__GET_CONTEXT();
	gc->mPM.CompileShader(gc, shader);
}

GLAPI void APIENTRY glAttachShader (GLuint program, GLuint shader)
{
	__GET_CONTEXT();
	gc->mPM.AttachShader(gc, program, shader);
}

GLAPI void APIENTRY glUseProgram (GLuint program)
{
	__GET_CONTEXT();
	gc->mPM.UseProgram(gc, program);
}

GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name)
{
	__GET_CONTEXT();
	gc->mPM.GetUniformLocation(gc, program, name);
}

GLAPI void APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	__GET_CONTEXT();
	gc->mPM.GetUniformLocation(gc, location, count, transpose, value);
}

// vertex shader cache
Shader::Shader()
{
	mSource = NULL;
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
	Shader::ShaderType mType;
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

	if(mProgramNameSpace.genNames(1, &name))
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

// TODO: impl
void ProgramMachine::DeleteProgram(GLContext *gc, unsigned program)
{
	GLSP_UNREFERENCED_PARAM(gc);
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
	const char ** pString = const_cast<const char **>(string);
	ShaderFactory *pFact = reinterpret_cast<ShaderFactory *>(pString);
	ShaderPlaceHolder *pSPH = static_cast<ShaderPlaceHolder *>(mShaderNameSpace.retrieveObject(shader));

	if(pSPH->mType == Shader::VERTEX)
		pShader = pFact->createVertexShader();
	else if(pSPH->mType == Shader::FRAGMENT)
		pShader = pFact->createFragmentShader();

	if(!pShader)
		return;
	
	mShaderNameSpace.removeObject(pSPH);
	pShader->setType(pSPH->mType);
	pShader->setName(shader);
	pShader->shaderSource(pString);
	mShaderNameSpace.insertObject(pShader);
	delete pSPH;
}

void ProgramMachine::CompileShader(GLContext *gc, unsigned shader)
{
	GLSP_UNREFERENCED_PARAM(gc);
	return;
}

void ProgramMachine::AttachShader(GLContext *gc, unsigned program, unsigned shader)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));
	Shader *pShader = static_cast<Shader *>(mShaderNameSpace.retrieveObject(shader));

	if(!pProg || !pShader)
		return;
	
	pProg->attachShader(pShader);
}

void ProgramMachine::UseProgram(GLContext *gc, unsigned program)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));
	if(!pProg || !(pProg->validate()))
		return;

	mCurrentProgram = pProg;
}

int ProgramMachine::GetUniformLocation(GLContext, unsigned program, const char *name)
{
	int location1, location2;
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));

	if(!pProg)
		return -1;

	location1 = pProg->getVS()->GetUniformLocation(name);
	location2 = pProg->getFS()->GetUniformLocation(name);

	if(location1 == -1 && location2 == -1)
	{
		cout << __func__ << ": no such uniform in neither VS or FS!" << endl;
		return -1;
	}
	else if(location1 != -1 && location2 != -1)
	{
		cout << __func__ << ": uniform name conflict in VS and FS!" << endl;
		return -1;
	}
	else
		return (location1 != -1)? location1: location2;
}

void UniformValue(GLContext *gc, int location, int count, bool transpose, const float *value)
{
}
