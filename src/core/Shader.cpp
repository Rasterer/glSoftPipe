#include "Shader.h"
#include "GLContext.h"
#include "VertexCache.h"
#include "glcorearb.h"

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

GLAPI void APIENTRY glLinkProgram (GLuint program)
{
	__GET_CONTEXT();
	gc->mPM.LinkProgram(gc, program);
}

GLAPI void APIENTRY glUseProgram (GLuint program)
{
	__GET_CONTEXT();
	gc->mPM.UseProgram(gc, program);
}

GLAPI GLint APIENTRY glGetUniformLocation (GLuint program, const GLchar *name)
{
	__GET_CONTEXT();
	return gc->mPM.GetUniformLocation(gc, program, name);
}

GLAPI void APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	__GET_CONTEXT();
	gc->mPM.UniformValue(gc, location, count, transpose, reinterpret_cast<const mat4 *>(value));
}

GLAPI GLint APIENTRY glGetAttribLocation (GLuint program, const GLchar *name)
{
	__GET_CONTEXT();
	return gc->mPM.GetAttribLocation(gc, program, name);
}

// vertex shader cache
Shader::Shader()
{
	mSource = NULL;
}

Shader::ShaderType Shader::OGLToInternal(unsigned type)
{
	switch(type)
	{
		case GL_VERTEX_SHADER:		return VERTEX;
		case GL_FRAGMENT_SHADER:	return FRAGMENT;
		default:					return INVALID;
	}
}

void VertexShader::compile()
{
}

void VertexShader::execute(vsInput &in, vsOutput &out)
{
	out.outputSize(in.size());

	for(size_t i = 0; i < in.size(); i++)
	{
		out.getVarying(i) = in.getAttrib(i);
	}
}

void VertexShader::onExecute(vsInput &in, vsOutput &out)
{
}

void VertexShader::declareAttrib(const string &name, const type_info &type)
{
	mAttribMap[name] = mAttribRegs.size();
	mAttribRegs.push_back(PerVertVar(name, type));
}

int VertexShader::resolveAttrib(const string &name, const type_info &type)
{
	VarMap::iterator it = mAttribMap.find(name);
	if(it != mAttribMap.end())
	{
		assert(it->second < (int)mAttribRegs.size());
		assert(mAttribRegs[it->second].mType == type);
		return it->second;
	}

	return -1;
}

void VertexShader::declareVarying(const string &name, const type_info &type)
{
	mVaryingMap[name] = mVaryingRegs.size();
	mVaryingRegs.push_back(PerVertVar(name, type));
}

int VertexShader::resolveVarying(const string &name, const type_info &type)
{
	VarMap::iterator it = mVaryingMap.find(name);
	if(it != mVaryingMap.end())
	{
		assert(it->second < (int)mVaryingRegs.size());
		assert(mVaryingRegs[it->second].mType == type);
		return it->second;
	}

	return -1;
}

int VertexShader::GetAttribLocation(const string &name)
{
	VarMap::iterator it = mAttribMap.find(name);

	if(it != mAttribMap.end())
		return it->second;
	else
		return -1;
}

void VertexShader::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);
	vsCache &cache = bat->mCache;
	vsOutBuf &out = bat->mOut;
	vsCache::iterator it = cache.begin();
	vsOutBuf::iterator iter = out.begin();

	bat->mOut.resize(cache.size());

	while(it != cache.end())
	{
		execute(it->first, *iter);
		it++;
		iter++
	}

	getNextStage()->emit(bat);
}

void FragmentShader::compile()
{
}

void FragmentShader::attribPointer(float *attri)
{
	mIn = attri;
}

void FragmentShader::setupOutputRegister(char *outReg)
{
	mOutReg = outReg;
}

// TODO: Inherit PixelShader to do concrete implementation
void FragmentShader::execute()
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

void Program::LinkProgram()
{
	vUniform_t & VSUniform = mVertexShader->getUniformBlock();
	vUniform_t & FSUniform = mFragmentShader->getUniformBlock();

	if(validate() != true)
		return;

	// TODO: add uniform conflict check
	vUniform_t::iterator it = VSUniform.begin();
	while(it != VSUniform.end())
	{
		mUniformMap[it->mName] = mUniformRegs.size();
		mUniformRegs.push_back(*it++);
	}

	it = FSUniform.begin();
	while(it != FSUniform.end())
	{
		mUniformMap[it->mName] = mUniformRegs.size();
		mUniformRegs.push_back(*it++);
	}
}

int Program::GetUniformLocation(const string &name)
{
	UniformMap::iterator it = mUniformMap.find(name);

	if(it != mUniformMap.end())
		return it->second;
	else
		return -1;
}

ProgramMachine::ProgramMachine():
	mCurrentProgram(NULL)
{
}

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
	GLSP_UNREFERENCED_PARAM(shader);
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

void ProgramMachine::LinkProgram(GLContext *gc, unsigned program)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));

	if(!pProg)
		return;
	
	pProg->LinkProgram();
}

void ProgramMachine::UseProgram(GLContext *gc, unsigned program)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));
	if(!pProg)
		return;

	mCurrentProgram = pProg;
}

int ProgramMachine::GetUniformLocation(GLContext *gc, unsigned program, const char *name)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));

	if(!pProg)
		return -1;

	return pProg->GetUniformLocation(name);
}

int ProgramMachine::GetAttribLocation(GLContext *gc, unsigned program, const char *name)
{
	GLSP_UNREFERENCED_PARAM(gc);

	Program *pProg = static_cast<Program *>(mProgramNameSpace.retrieveObject(program));

	if(!pProg)
		return -1;

	return pProg->getVS()->GetAttribLocation(name);
}
