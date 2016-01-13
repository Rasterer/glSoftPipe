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
#include "Shader.h"
#include "RenderUtility.h"


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

	virtual void OnExecuteSIMD(__m128 in[], __m128 out[])
	{
		texture2D(mSampler, in[4], in[4 + 1], out);
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

	ShaderFactory *mShaderFactory;
	float mScalar;
	GLuint mProg;
	GLint mWVPLocation;
	mat4  mTVP;
	Mesh *mMesh;
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

	mat4 trans = ::glm::translate(mat4(1.0f), vec3(0.0f, -20.0f, -100.0f));
	mat4 view = ::glm::lookAt(vec3(0.0f, 0.0f, 0.0f),
							vec3(0.0f, 0.0f, -1.0f),
							vec3(0.0f, 1.0f, 0.0f));
	mat4 project = ::glm::perspective((float)M_PI * 60.0f / 180.0f, 16.0f / 9.0f, 10.0f, 200.0f);
	mTVP = project * view * trans;

	glUniform1i(samplerLocation, 0);

	mMesh = new Mesh();
	mMesh->LoadMesh("../../../examples/LoadModel/materials/phoenix_ugv.md2");
}

void LoadModel::onRender()
{
	mat4 rotate = ::glm::rotate(mat4(1.0f), (float)M_PI * mScalar / 180, vec3(0.0f, 1.0f, 0.0f));
	mat4 wvp = mTVP * rotate;
	glUniformMatrix4fv(mWVPLocation, 1, false, (float *)&wvp);
	mScalar += 1.0f;

	mMesh->Render();

#if 0

	std::chrono::high_resolution_clock::time_point  beginTime = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point  endTime;
	while(true)
	{
		rotate = glm::rotate(mat4(1.0f), (float)M_PI * scalar / 180, vec3(0.0f, 1.0f, 0.0f));

		wvp   = tvp * rotate * scal;
		glUniformMatrix4fv(WVPLocation, 1, false, (float *)&wvp);
		scalar += 1.0f;

		pMesh->Render();
		ok = eglSwapBuffers(display, surface);

		nFrames++;

		if(nFrames == 150)
		{
			endTime = std::chrono::high_resolution_clock::now();

			printf("FPS: %f\n", 150.0 / (std::chrono::duration_cast<std::chrono::duration<double>>(endTime - beginTime)).count());

			nFrames = 0;
			beginTime = endTime;
		}
	}
#endif
}

} // namespace glsp

int main(int argc, char *argv[])
{
	Magick::InitializeMagick(*argv);
	glsp::LoadModel app;
	app.run();
}