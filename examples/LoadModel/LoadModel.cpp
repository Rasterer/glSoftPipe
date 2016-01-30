#include <chrono>

#define _USE_MATH_DEFINES
#include <cmath>
#include <string.h>
#include <thread>
#include <chrono>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "IAppFramework.h"
#include "DataFlow.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"


using namespace std;
using namespace glm;


namespace glsp {
#include "khronos/GL/glcorearb.h"

class simpleVertexShader: public VertexShader
{
public:
	simpleVertexShader()
	{
		DECLARE_UNIFORM(mWVP);

		DECLARE_IN(vec3, iPos);
		DECLARE_TEXTURE_COORD(vec2, iTexCoor);

		DECLARE_OUT(vec4, gl_Position);
		DECLARE_OUT(vec2, oTexCoor);
	}

	void execute(vsInput &in, vsOutput &out)
	{
		RESOLVE_IN(vec3, iPos, in);
		RESOLVE_IN(vec2, iTexCoor, in);

		RESOLVE_OUT(vec4, gl_Position, out);
		RESOLVE_OUT(vec2, oTexCoor, out);

		gl_Position = mWVP * vec4(iPos, 1.0f);
		oTexCoor    = iTexCoor;
	}

private:
	mat4  mWVP;
	GLint miPos;
	GLint miTexCoor;
	GLint mgl_Position;
	GLint moTexCoor;
};

class simpleFragmentShader: public FragmentShader
{
public:
	simpleFragmentShader()
	{
		DECLARE_SAMPLER(mSampler);

		DECLARE_IN(vec4, gl_Position);
		DECLARE_TEXTURE_COORD(vec2, oTexCoor);

		DECLARE_OUT(vec4, FragColor);
	}

private:
	void execute(fsInput &in, fsOutput &out)
	{
		RESOLVE_IN(vec4, gl_Position, in);
		RESOLVE_IN(vec2, oTexCoor, in);

		RESOLVE_OUT(vec4, FragColor, out);

		(void)gl_Position;

		FragColor = texture2D(mSampler, oTexCoor);
	}

	virtual void OnExecuteSIMD(Fsiosimd &fsio)
	{
		texture2D(mSampler, fsio.mInRegs[4], fsio.mInRegs[4 + 1], fsio.mOutRegs);

		_MM_TRANSPOSE4_PS(fsio.mOutRegs[0], fsio.mOutRegs[1], fsio.mOutRegs[2], fsio.mOutRegs[3]);
	}

private:
	sampler2D mSampler;
	GLint     mgl_Position;
	GLint     moTexCoor;
	GLint     mFragColor;
};

class simpleShaderFactory: public ShaderFactory
{
public:
	Shader *createVertexShader()
	{
		mVS = new simpleVertexShader();
		return mVS;
	}

	void DeleteVertexShader(Shader *pVS)
	{
		delete mVS;
	}

	Shader *createFragmentShader()
	{
		mFS = new simpleFragmentShader();
		return mFS;
	}

	void DeleteFragmentShader(Shader *pFS)
	{
		delete mFS;
	}
};


class LoadModel: public GlspApp
{
public:
	LoadModel() = default;
	~LoadModel();

private:
	virtual void onInit();
	virtual void onRender();
	virtual void onKeyPressed(unsigned long key);
	virtual void onMouseLeftClickDown(int x, int y);

	ShaderFactory *mShaderFactory;
	float mScalar;
	GLuint mProg;
	GLint mWVPLocation;

	mat4  mTrans;
	mat4  mProject;
	mat4  mTransViewProject;

	GlspCamera mCamera;
	GlspMesh *mMesh;
};

LoadModel::~LoadModel()
{
	if (mProg)          glDeleteProgram(mProg);
	if (mMesh)          delete mMesh;
	if (mShaderFactory) delete mShaderFactory;
}

void LoadModel::onInit()
{
	setWindowInfo(1280, 720, "LoadModel");

	mShaderFactory = new simpleShaderFactory();

	mProg = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vs, 0, reinterpret_cast<const char *const*>(mShaderFactory), 0);
	glShaderSource(fs, 0, reinterpret_cast<const char *const*>(mShaderFactory), 0);

	glCompileShader(vs);
	glCompileShader(fs);

	glAttachShader(mProg, vs);
	glAttachShader(mProg, fs);
	glLinkProgram(mProg);
	glDetachShader(mProg, vs);
	glDetachShader(mProg, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	glUseProgram(mProg);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	mWVPLocation = glGetUniformLocation(mProg, "mWVP");
	GLint samplerLocation = glGetUniformLocation(mProg, "mSampler");

	mCamera.InitCamera(vec3(0.0f, 0.0f, 0.0f),
					vec3(0.0f, 0.0f, -1.0f),
					vec3(0.0f, 1.0f, 0.0f));

	mTrans   = ::glm::translate(mat4(1.0f), vec3(0.0f, -20.0f, -100.0f));
	mProject = ::glm::perspective((float)M_PI * 60.0f / 180.0f, 16.0f / 9.0f, 10.0f, 200.0f);
	mTransViewProject = mProject * mCamera.GetViewMatrix() * mTrans;

	glUniform1i(samplerLocation, 0);

	mMesh = new GlspMesh();
	mMesh->LoadMesh("../../../examples/LoadModel/materials/phoenix_ugv.md2");
}

void LoadModel::onRender()
{
	mat4 rotate = ::glm::rotate(mat4(1.0f), (float)M_PI * mScalar / 180, vec3(0.0f, 1.0f, 0.0f));
	mat4 wvp = mTransViewProject * rotate;
	glUniformMatrix4fv(mWVPLocation, 1, false, (float *)&wvp);
	mScalar += 1.0f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mMesh->Render();
}

void LoadModel::onKeyPressed(unsigned long key)
{
	if (mCamera.CameraControl(key))
	{
		// This key event has been processed by camera control,
		// need update the TVP matrix.
		mTransViewProject = mProject * mCamera.GetViewMatrix() * mTrans;
	}
}

void LoadModel::onMouseLeftClickDown(int x, int y)
{
	//printf("Mouse left click: %d %d", x, y);
}


} // namespace glsp

int main(int argc, char *argv[])
{
	Magick::InitializeMagick(*argv);
	glsp::LoadModel app;
	app.run();
}