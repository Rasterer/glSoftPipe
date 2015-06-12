#include "Shader.h"

#include "GLContext.h"
#include "DataFlow.h"
#include "DrawEngine.h"
#include "Rasterizer.h"
#include "Texture.h"
#include "SamplerObject.h"
#include "khronos/GL/glcorearb.h"


using glm::vec4;
using glm::vec2;
using glm::mat4;

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

GLAPI void APIENTRY glUniform1i (GLint location, GLint v0)
{
	__GET_CONTEXT();
	gc->mPM.UniformUif(gc, location, 1, &v0);
}

GLAPI void APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	__GET_CONTEXT();
	gc->mPM.UniformMatrix(gc, location, count, transpose, reinterpret_cast<const mat4 *>(value));
}

GLAPI GLint APIENTRY glGetAttribLocation (GLuint program, const GLchar *name)
{
	__GET_CONTEXT();
	return gc->mPM.GetAttribLocation(gc, program, name);
}


NS_OPEN_GLSP_OGL()

// vertex shader cache
Shader::Shader():
	mSource(NULL),
	bHasSampler(false),
	mNumSamplers(0)
{
}

Shader::ShaderType Shader::OGLShaderTypeToInternal(unsigned type)
{
	switch(type)
	{
		case GL_VERTEX_SHADER:		return VERTEX;
		case GL_FRAGMENT_SHADER:	return FRAGMENT;
		default:					return INVALID;
	}
}

void Shader::declareInput(const string &name, const type_info &type)
{
	mInRegsMap[name] = mInRegs.size();
	mInRegs.push_back(VertexInfo(name, type));

	assert(mInRegs.size() <= MAX_VERTEX_ATTRIBS);
}

int Shader::resolveInput(const string &name, const type_info &type)
{
	VarMap::iterator it = mInRegsMap.find(name);
	if(it != mInRegsMap.end())
	{
		assert(it->second < (int)mInRegs.size());
		assert(mInRegs[it->second].mType == type);
		return it->second;
	}

	return -1;
}

void Shader::declareOutput(const string &name, const type_info &type)
{
	mOutRegsMap[name] = mOutRegs.size();
	mOutRegs.push_back(VertexInfo(name, type));
}

int Shader::resolveOutput(const string &name, const type_info &type)
{
	VarMap::iterator it = mOutRegsMap.find(name);
	if(it != mOutRegsMap.end())
	{
		assert(it->second < (int)mOutRegs.size());
		assert(mOutRegs[it->second].mType == type);
		return it->second;
	}

	return -1;
}

void Shader::declareSampler()
{
	assert(mNumSamplers < kMaxSamplers);
	mSamplerLoc[mNumSamplers++] = mUniformBlock.size();

	if(!bHasSampler)
		bHasSampler = true;
}

int Shader::GetInRegLocation(const string &name)
{
	VarMap::iterator it = mInRegsMap.find(name);

	if(it != mInRegsMap.end())
		return it->second;
	else
		return -1;
}

unsigned Shader::getSamplerUnitID(int i) const
{
	unsigned unit;
	mUniformBlock[mSamplerLoc[i]].getVal(&unit);
	return unit;
}

static inline vec4 BiLinearInterpolate(float scaleX, float scaleY, vec4 lb, vec4 lt, vec4 rb, vec4 rt)
{
	return (lb *= ((1.0 - scaleX) * (1.0f - scaleY))) +
		   (lt *= ((1.0 - scaleX) * scaleY)) +
		   (rb *= (scaleX * (1.0f - scaleY))) +
		   (rt *= (scaleX * scaleY));
}

// Calculate the "Level of Detail"
// TODO: calculate the partial derivatives
int Shader::calculateLOD(vec2 coord)
{
	return 0;
}

vec4 Shader::texture2D(sampler2D sampler, vec2 coord)
{
	int lvl = calculateLOD(coord);
	TextureMipmap *pMipmap = mTexs[sampler]->getMipmap(0, lvl);
	const SamplerObject &so = mTexs[sampler]->getSamplerObject();

	int x, y;
	float TexSpaceX = coord.x * pMipmap->mWidth;
	float TexSpaceY = coord.y * pMipmap->mHeight;
	vec4 *pAddr = static_cast<vec4 *>(pMipmap->mMem.addr);

	if(so.eMagFilter == GL_NEAREST)
	{
		x = (int)floor(TexSpaceX);
		y = (int)floor(TexSpaceY);

		return pAddr[(pMipmap->mHeight - y - 1) * pMipmap->mWidth + x];
	}
	else //GL_LINEAR
	{
		x = (int)floor(TexSpaceX - 0.5f);
		y = (int)floor(TexSpaceY - 0.5f);

		vec4 lb = pAddr[(pMipmap->mHeight - y - 1) * pMipmap->mWidth + x];
		vec4 lt = pAddr[(pMipmap->mHeight - y) * pMipmap->mWidth + x];
		vec4 rb = pAddr[(pMipmap->mHeight - y - 1) * pMipmap->mWidth + x + 1];
		vec4 rt = pAddr[(pMipmap->mHeight - y) * pMipmap->mWidth + x];

		float scaleX = TexSpaceX - 0.5f - x;
		float scaleY = TexSpaceY - 0.5f - y;

		return BiLinearInterpolate(scaleX, scaleY, lb, lt, rb, rt);
	}
}

VertexInfo::VertexInfo(const string &name, const type_info &type):
	mName(name),
	mType(type)
{
}

void VertexShader::compile()
{
}

VertexShader::VertexShader():
	PipeStage("Vertex Shading", DrawEngine::getDrawEngine())
{
}

void VertexShader::execute(vsInput &in, vsOutput &out)
{
	out = in;
}

void VertexShader::emit(void *data)
{
	Batch *bat = static_cast<Batch *>(data);
	vsInput_v   &in = bat->mVertexCache;
	vsOutput_v &out = bat->mVsOut;

	out.resize(in.size());

	for(size_t i = 0; i < in.size(); i++)
	{
		out[i].resize(getOutRegsNum());
		execute(in[i], out[i]);
	}

	// Free the memory in Batch.mVertexCache to avoid large memory occupy
	vsInput_v().swap(in);

	getNextStage()->emit(bat);
}

void VertexShader::finalize()
{
}

FragmentShader::FragmentShader():
	PipeStage("Fragment Shading", DrawEngine::getDrawEngine()),
	bHasDiscard(false)
{
}

void FragmentShader::emit(void *data)
{
	Fsio *pFsio = static_cast<Fsio *>(data);

	pFsio->out.resize(getOutRegsNum());
	execute(pFsio->in, pFsio->out);

	getNextStage()->emit(pFsio);
}

void FragmentShader::compile()
{
}

// Inherit PixelShader to do concrete implementation
void FragmentShader::execute(fsInput& in, fsOutput& out)
{
	GLSP_UNREFERENCED_PARAM(in);

	out.fragcolor() = vec4(0.0f, 255.0f, 0.0f, 255.0f);
}

void FragmentShader::finalize()
{
}

class ShaderPlaceHolder: public NameItem
{
public:
	Shader::ShaderType mType;
};

Program::Program():
	mVertexShader(NULL),
	mFragmentShader(NULL)
{
}

void Program::AttachShader(Shader *pShader)
{
	if(pShader->getType() == Shader::VERTEX)
		mVertexShader = static_cast<VertexShader *>(pShader);
	else if(pShader->getType() == Shader::FRAGMENT)
		mFragmentShader = static_cast<FragmentShader *>(pShader);
	else
		assert(false);
}

bool Program::validate()
{
	if(mVertexShader && mFragmentShader)
		return true;
	else
		return false;
}

void Program::LinkProgram()
{
	uniform_v & VSUniform = mVertexShader->getUniformBlock();
	uniform_v & FSUniform = mFragmentShader->getUniformBlock();

	if(validate() != true)
		return;

	// TODO: add uniform conflict check
	uniform_v::iterator it = VSUniform.begin();
	while(it != VSUniform.end())
	{
		mUniformMap[it->mName] = mUniformBlock.size();
		mUniformBlock.push_back(*it++);
	}

	it = FSUniform.begin();
	while(it != FSUniform.end())
	{
		mUniformMap[it->mName] = mUniformBlock.size();
		mUniformBlock.push_back(*it++);
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
		pSPH->mType = Shader::OGLShaderTypeToInternal(type);
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
	pShader->setSource(pString);
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
	
	pProg->AttachShader(pShader);
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

	setCurrentProgram(pProg);
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

	return pProg->getVS()->GetInRegLocation(name);
}

NS_CLOSE_GLSP_OGL()
