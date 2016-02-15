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
#include "utils.h"
#include "khronos/GL/glspcorearb.h"


using namespace std;
using namespace glm;


namespace glsp {

class LightingVertexShader: public VertexShader
{
public:
	LightingVertexShader()
	{
		DECLARE_UNIFORM(mWVP);
		DECLARE_UNIFORM(mWorld);

		DECLARE_IN(vec3, iPos);
		DECLARE_TEXTURE_COORD(vec2, iTexCoor);
		DECLARE_IN(vec3, iNormal);

		DECLARE_OUT(vec4, gl_Position);
		DECLARE_OUT(vec2, oTexCoor);
		DECLARE_OUT(vec3, oNormal);
		DECLARE_OUT(vec4, oWorldCoord);
	}

	void execute(vsInput &in, vsOutput &out)
	{
		RESOLVE_IN(vec3, iPos, in);
		RESOLVE_IN(vec2, iTexCoor, in);
		RESOLVE_IN(vec3, iNormal, in);

		RESOLVE_OUT(vec4, gl_Position, out);
		RESOLVE_OUT(vec2, oTexCoor, out);
		RESOLVE_OUT(vec3, oNormal, out);
		RESOLVE_OUT(vec4, oWorldCoord, out);

		gl_Position = mWVP * vec4(iPos, 1.0f);
		oTexCoor    = iTexCoor;
		vec4 normal = mWorld * vec4(iNormal, 0.0f);
		oNormal     = vec3(normal.x, normal.y, normal.z);
		oWorldCoord = mWorld * vec4(iPos, 1.0f);
	}

private:
	mat4  mWVP;
	mat4  mWorld;
	GLint miPos;
	GLint miTexCoor;
	GLint miNormal;
	GLint mgl_Position;
	GLint moTexCoor;
	GLint moNormal;
	GLint moWorldCoord;
};

struct DirectionalLight
{
	vec3  Color;
	vec3  Direction;
	float AmbientIntensity;
	float DiffuseIntensity;
};

struct PointLight
{
	vec3  Color;
	vec3  Position;
	float AmbientIntensity;
	float DiffuseIntensity;
	float SpecularIntensity;
	float AttenuationConstant;
	float AttenuationLinear;
	float AttenuationSquare;
};

class LightingFragmentShader: public FragmentShader
{
public:
	LightingFragmentShader()
	{
		DECLARE_UNIFORM(mPointLight);
		DECLARE_UNIFORM(mEyePos);
		DECLARE_SAMPLER(mSampler);

		DECLARE_IN(vec4, gl_Position);
		DECLARE_TEXTURE_COORD(vec2, oTexCoor);
		DECLARE_IN(vec3, oNormal);
		DECLARE_IN(vec4, oWorldCoord);

		DECLARE_OUT(vec4, FragColor);
	}

private:
	virtual void OnExecuteSIMD(Fsiosimd &fsio)
	{
		__m128 vNormalizedX = fsio.mInRegs[moNormal + 0];
		__m128 vNormalizedY = fsio.mInRegs[moNormal + 1];
		__m128 vNormalizedZ = fsio.mInRegs[moNormal + 2];
		normalize(vNormalizedX, vNormalizedY, vNormalizedZ);

		__m128 vLightToVertexX = _mm_sub_ps(fsio.mInRegs[moWorldCoord + 0], _mm_set_ps1(mPointLight.Position.x));
		__m128 vLightToVertexY = _mm_sub_ps(fsio.mInRegs[moWorldCoord + 1], _mm_set_ps1(mPointLight.Position.y));
		__m128 vLightToVertexZ = _mm_sub_ps(fsio.mInRegs[moWorldCoord + 2], _mm_set_ps1(mPointLight.Position.z));
		__m128 vDistance       = length(vLightToVertexX, vLightToVertexY, vLightToVertexZ);
		normalize(vLightToVertexX, vLightToVertexY, vLightToVertexZ);

		__m128 vNormalLightDot = dot(vNormalizedX, vNormalizedY, vNormalizedZ, vLightToVertexX, vLightToVertexY, vLightToVertexZ);
		__m128 vMask = _mm_cmp_ps(vNormalLightDot, _mm_setzero_ps(), _CMP_LT_OS);
		vNormalLightDot = _mm_and_ps(vNormalLightDot, vMask);

		__m128 vSpecularFactor;
		__m128 vDiffuseFactor;
		if (_mm_test_all_zeros(_mm_castps_si128(vNormalLightDot), _mm_set1_epi32(0xFFFFFFFF)))
		{
			vDiffuseFactor  = _mm_setzero_ps();
			vSpecularFactor = _mm_setzero_ps();
		}
		else
		{
			// reverse the sign bit, because we use incident light to calculate
			// the factor above.
			vDiffuseFactor = _mm_xor_ps(vNormalLightDot, _mm_set_ps1(-0.0f));
			vDiffuseFactor = _mm_mul_ps(vDiffuseFactor, _mm_set_ps1(mPointLight.DiffuseIntensity));

			__m128 vVectexToEyeX = _mm_sub_ps(_mm_set_ps1(mEyePos.x), fsio.mInRegs[moWorldCoord + 0]);
			__m128 vVectexToEyeY = _mm_sub_ps(_mm_set_ps1(mEyePos.y), fsio.mInRegs[moWorldCoord + 1]);
			__m128 vVectexToEyeZ = _mm_sub_ps(_mm_set_ps1(mEyePos.z), fsio.mInRegs[moWorldCoord + 2]);
			normalize(vVectexToEyeX, vVectexToEyeY, vVectexToEyeZ);

			__m128 vReflectDirectionX;
			__m128 vReflectDirectionY;
			__m128 vReflectDirectionZ;
			reflect(vLightToVertexX, vLightToVertexY, vLightToVertexZ,
					vNormalizedX, vNormalizedY, vNormalizedZ,
					vReflectDirectionX, vReflectDirectionY, vReflectDirectionZ);
			normalize(vReflectDirectionX, vReflectDirectionY, vReflectDirectionZ);

			vSpecularFactor = dot(vReflectDirectionX, vReflectDirectionY, vReflectDirectionZ,
								vVectexToEyeX, vVectexToEyeY, vVectexToEyeZ);
			__m128 vMask1 = _mm_cmp_ps(vSpecularFactor, _mm_setzero_ps(), _CMP_GT_OS);
			vSpecularFactor = _mm_and_ps(_mm_and_ps(vSpecularFactor, vMask1), vMask);
			vSpecularFactor = _mm_mul_ps(vSpecularFactor, _mm_set_ps1(mPointLight.SpecularIntensity));
		}

		__m128 vAmbientFactor = _mm_set_ps1(mPointLight.AmbientIntensity);
		__m128 vFactor = _mm_add_ps(_mm_add_ps(vDiffuseFactor, vSpecularFactor), vAmbientFactor);

		__m128 vAttenuationConstant = _mm_set_ps1(mPointLight.AttenuationConstant);
		__m128 vAttenuationLinear   = _mm_mul_ps(vDistance, _mm_set_ps1(mPointLight.AttenuationLinear));
		__m128 vAttenuationSquare   = _mm_mul_ps(_mm_mul_ps(vDistance, vDistance), _mm_set_ps1(mPointLight.AttenuationSquare));

		__m128 vAttenuation = _mm_add_ps(_mm_add_ps(vAttenuationConstant, vAttenuationLinear), vAttenuationSquare);

		vFactor = _mm_div_ps(vFactor, vAttenuation);

		texture2D(mSampler, fsio, fsio.mInRegs[moTexCoor + 0], fsio.mInRegs[moTexCoor + 1], fsio.mOutRegs);
		fsio.mOutRegs[0] = _mm_mul_ps(_mm_mul_ps(fsio.mOutRegs[0], vFactor), _mm_set_ps1(mPointLight.Color.x));
		fsio.mOutRegs[1] = _mm_mul_ps(_mm_mul_ps(fsio.mOutRegs[1], vFactor), _mm_set_ps1(mPointLight.Color.y));
		fsio.mOutRegs[2] = _mm_mul_ps(_mm_mul_ps(fsio.mOutRegs[2], vFactor), _mm_set_ps1(mPointLight.Color.z));
		fsio.mOutRegs[3] = _mm_mul_ps(_mm_mul_ps(fsio.mOutRegs[3], vFactor), _mm_set_ps1(1.0f));
		_MM_TRANSPOSE4_PS(fsio.mOutRegs[mFragColor + 0], fsio.mOutRegs[mFragColor + 1], fsio.mOutRegs[mFragColor + 2], fsio.mOutRegs[mFragColor + 3]);
	}

private:
	PointLight mPointLight;
	vec3       mEyePos;
	sampler2D  mSampler;

	GLint     mgl_Position;
	GLint     moTexCoor;
	GLint     moNormal;
	GLint     moWorldCoord;
	GLint     mFragColor;
};

class LightingShaderFactory: public ShaderFactory
{
public:
	Shader *createVertexShader()
	{
		mVS = new LightingVertexShader();
		return mVS;
	}

	void DeleteVertexShader(Shader *pVS)
	{
		delete mVS;
	}

	Shader *createFragmentShader()
	{
		mFS = new LightingFragmentShader();
		return mFS;
	}

	void DeleteFragmentShader(Shader *pFS)
	{
		delete mFS;
	}
};

class CrytekSponza: public GlspApp
{
public:
	CrytekSponza() = default;
	~CrytekSponza();

private:
	virtual bool onInit();
	virtual void onRender();
	virtual void onKeyPressed(unsigned long key);
	virtual void onMouseLeftClickDown(int x, int y);

	ShaderFactory *mShaderFactory;
	float mScalar;

	GLuint mProg;
	GLint mWVPLocation;
	GLint mWorldLocation;

	mat4  mWorld;
	mat4  mProject;
	mat4  mWVP;

	GlspCamera     mCamera;
	GlspMesh      *mMesh;
};

CrytekSponza::~CrytekSponza()
{
	if (mProg)          glDeleteProgram(mProg);
	if (mMesh)          delete mMesh;
	if (mShaderFactory) delete mShaderFactory;
}

bool CrytekSponza::onInit()
{
#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 1020
	setWindowInfo(WINDOW_WIDTH, WINDOW_HEIGHT, "CrytekSponza");

	mShaderFactory = new LightingShaderFactory();

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

	mWVPLocation = glGetUniformLocation(mProg, "mWVP");
	mWorldLocation = glGetUniformLocation(mProg, "mWorld");
	GLint samplerLocation = glGetUniformLocation(mProg, "mSampler");
	glUniform1i(samplerLocation, 0);

	vec3 light_position = vec3(0.0f, 10.0f, 0.0f);

	PointLight *point_light = static_cast<PointLight *>(glspGetUniformLocation(mProg, "mPointLight"));
	point_light->Color = vec3(1.0f, 1.0f, 1.0f);
	point_light->Position = light_position;
	point_light->AttenuationConstant = 1.0f;
	point_light->AttenuationLinear = 0.01f;
	point_light->AttenuationSquare = 0.0001f;
	point_light->AmbientIntensity = 0.8f;
	point_light->DiffuseIntensity = 0.2f;
	point_light->SpecularIntensity = 1.0f;

	vec3 *eye_pos = static_cast<vec3 *>(glspGetUniformLocation(mProg, "mEyePos"));
	*eye_pos = vec3(0.0f, 0.0f, 0.0f);

	mCamera.InitCamera(*eye_pos,
					vec3(-1.0f, 0.0f, 0.0f),
					vec3(0.0f, 1.0f, 0.0f));

	mat4 trans = glm::translate(mat4(1.0f), vec3(0.0f, -10.0f, 0.0f));
	mat4 scale = glm::scale(mat4(1.0f), vec3(0.03f, 0.03f, 0.03f));
	mWorld  = trans * scale;

	mProject = glm::perspective((float)M_PI * 60.0f / 180.0f, 16.0f / 9.0f, 1.0f, 500.0f);
	mWVP = mProject * mCamera.GetViewMatrix() * mWorld;

	mMesh = new GlspMesh();
	mMesh->LoadMesh(GLSP_ROOT "assets/crytek_sponza/sponza.obj");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	return true;
}

void CrytekSponza::onRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(mWVPLocation, 1, false, (float *)&mWVP);
	glUniformMatrix4fv(mWorldLocation, 1, false, (float *)&mWorld);

	mMesh->Render();
}

void CrytekSponza::onKeyPressed(unsigned long key)
{
	if (mCamera.CameraControl(key))
	{
		// This key event has been processed by camera control,
		// need update the TVP matrix.
		mWVP = mProject * mCamera.GetViewMatrix() * mWorld;
	}
}

void CrytekSponza::onMouseLeftClickDown(int x, int y)
{
	// printf("Mouse left click: %d %d", x, y);
}


} // namespace glsp

int main(int argc, char *argv[])
{
	Magick::InitializeMagick(*argv);
	glsp::CrytekSponza app;
	app.run();
}
