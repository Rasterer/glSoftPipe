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

class simpleVertexShader: public VertexShader
{
public:
	simpleVertexShader()
	{
		DECLARE_UNIFORM(mWVP);
		DECLARE_UNIFORM(mWorld);
		DECLARE_UNIFORM(mWVPFromLight);

		DECLARE_IN(vec3, iPos);
		DECLARE_TEXTURE_COORD(vec2, iTexCoor);
		DECLARE_IN(vec3, iNormal);

		DECLARE_OUT(vec4, gl_Position);
		DECLARE_OUT(vec2, oTexCoor);
		DECLARE_OUT(vec3, oNormal);
		DECLARE_OUT(vec4, oWorldCoord);
		DECLARE_OUT(vec4, oLightSpacePos);
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
		RESOLVE_OUT(vec4, oLightSpacePos, out);

		gl_Position = mWVP * vec4(iPos, 1.0f);
		oTexCoor    = iTexCoor;
		vec4 normal = mWorld * vec4(iNormal, 0.0f);
		oNormal     = vec3(normal.x, normal.y, normal.z);
		oWorldCoord = mWorld * vec4(iPos, 1.0f);
		oLightSpacePos = mWVPFromLight * vec4(iPos, 1.0f);
	}

private:
	mat4  mWVP;
	mat4  mWorld;
	mat4  mWVPFromLight;
	GLint miPos;
	GLint miTexCoor;
	GLint miNormal;
	GLint mgl_Position;
	GLint moTexCoor;
	GLint moNormal;
	GLint moWorldCoord;
	GLint moLightSpacePos;
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

class simpleFragmentShader: public FragmentShader
{
public:
	simpleFragmentShader()
	{
		DECLARE_UNIFORM(mPointLight);
		DECLARE_UNIFORM(mEyePos);
		DECLARE_SAMPLER(mSampler);
		DECLARE_SAMPLER(mShadowMapSampler);

		DECLARE_IN(vec4, gl_Position);
		DECLARE_TEXTURE_COORD(vec2, oTexCoor);
		DECLARE_IN(vec3, oNormal);
		DECLARE_IN(vec4, oWorldCoord);
		DECLARE_IN(vec4, oLightSpacePos);

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

		__m128 vWRecip = _mm_rcp_ps(fsio.mInRegs[moLightSpacePos + 3]);
		__m128 vLightSpaceX = _mm_mul_ps(fsio.mInRegs[moLightSpacePos + 0], vWRecip);
		__m128 vLightSpaceY = _mm_mul_ps(fsio.mInRegs[moLightSpacePos + 1], vWRecip);
		__m128 vLightSpaceZ = _mm_mul_ps(fsio.mInRegs[moLightSpacePos + 2], vWRecip);
		vLightSpaceZ = _mm_mul_ps(_mm_add_ps(vLightSpaceZ, _mm_set_ps1(1.0f)), _mm_set_ps1(0.5f));

		__m128 vShadowMapTexCoordS = _mm_mul_ps(_mm_add_ps(vLightSpaceX, _mm_set_ps1(1.0f)), _mm_set_ps1(0.5f));
		__m128 vShadowMapTexCoordT = _mm_mul_ps(_mm_add_ps(vLightSpaceY, _mm_set_ps1(1.0f)), _mm_set_ps1(0.5f));

		__m128 vClosestDepth[1];
		texture2D(mShadowMapSampler, fsio, vShadowMapTexCoordS, vShadowMapTexCoordT, vClosestDepth);

		// Add depth bias to overcome shadow acne.
		__m128 vBias = _mm_max_ps(_mm_mul_ps(_mm_add_ps(_mm_set_ps1(1.0f), vNormalLightDot), _mm_set_ps1(0.035f)), _mm_set_ps1(0.001f));
		__m128 vShadowMask = _mm_cmp_ps(_mm_sub_ps(vLightSpaceZ, vBias), vClosestDepth[0], _CMP_GT_OS);
		__m128 vShadowAdjust = _mm_sub_ps(_mm_set_ps1(1.0f), _mm_and_ps(vShadowMask, _mm_set_ps1(0.8f)));

		__m128 vAmbientFactor = _mm_set_ps1(mPointLight.AmbientIntensity);
		__m128 vFactor = _mm_add_ps(_mm_mul_ps(_mm_add_ps(vDiffuseFactor, vSpecularFactor), vShadowAdjust), vAmbientFactor);

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
	sampler2D  mShadowMapSampler;

	GLint     mgl_Position;
	GLint     moTexCoor;
	GLint     moNormal;
	GLint     moWorldCoord;
	GLint     moLightSpacePos;
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

class ShadowMapVertexShader: public VertexShader
{
public:
	ShadowMapVertexShader()
	{
		DECLARE_UNIFORM(mWVP);

		DECLARE_IN(vec3, iPos);

		DECLARE_OUT(vec4, gl_Position);
	}

	void execute(vsInput &in, vsOutput &out)
	{
		RESOLVE_IN(vec3, iPos, in);

		RESOLVE_OUT(vec4, gl_Position, out);

		gl_Position = mWVP * vec4(iPos, 1.0f);
	}

private:
	mat4  mWVP;
	GLint miPos;
	GLint mgl_Position;
};

class ShadowMapFragmentShader: public FragmentShader
{
public:
	ShadowMapFragmentShader()
	{
		DECLARE_IN(vec4, gl_Position);

		DECLARE_OUT(vec4, FragColor);
	}

private:
	virtual void OnExecuteSIMD(Fsiosimd &fsio)
	{
	}

private:
	GLint     mgl_Position;
	GLint     mFragColor;
};

class ShadowMapShaderFactory: public ShaderFactory
{
public:
	Shader *createVertexShader()
	{
		mVS = new ShadowMapVertexShader();
		return mVS;
	}

	void DeleteVertexShader(Shader *pVS)
	{
		delete mVS;
	}

	Shader *createFragmentShader()
	{
		mFS = new ShadowMapFragmentShader();
		return mFS;
	}

	void DeleteFragmentShader(Shader *pFS)
	{
		delete mFS;
	}
};

class ShadowMap: public GlspApp
{
public:
	ShadowMap() = default;
	~ShadowMap();

private:
	virtual bool onInit();
	virtual void onRender();
	virtual void onKeyPressed(unsigned long key);
	virtual void onMouseLeftClickDown(int x, int y);

	ShaderFactory *mShaderFactory;
	ShaderFactory *mShadowMapShaderFactory;
	float mScalar;

	GLuint mProg;
	GLint mWVPLocation;
	GLint mWorldLocation;
	GLint mWVPFromLightLocation;

	GLuint mShadowMapProg;
	GLint mShadowMapProgWVPLoc;

	GLuint mFBO;
	GLuint mShadowMap;

	mat4  mTrans;
	mat4  mProject;
	mat4  mViewProject;
	mat4  mQuadTransformWorld;
	mat4  mQuadTransformWVP;
	mat4  mLightVP;
	mat4  mQuadTransformWVPFromLight;

	GlspCamera     mCamera;
	GlspMesh      *mQuadMesh;
	GlspMesh      *mMesh;
	GlspMaterials *mQuadTexture;
};

ShadowMap::~ShadowMap()
{
	if (mProg)          glDeleteProgram(mProg);
	if (mMesh)          delete mMesh;
	if (mQuadMesh)      delete mQuadMesh;
	if (mShaderFactory) delete mShaderFactory;
}

bool ShadowMap::onInit()
{
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
	setWindowInfo(WINDOW_WIDTH, WINDOW_HEIGHT, "ShadowMap");

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

	mShadowMapShaderFactory = new ShadowMapShaderFactory();

	mShadowMapProg = glCreateProgram();
	GLuint sm_vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint sm_fs = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(sm_vs, 0, reinterpret_cast<const char *const*>(mShadowMapShaderFactory), 0);
	glShaderSource(fs, 0, reinterpret_cast<const char *const*>(mShadowMapShaderFactory), 0);

	glCompileShader(sm_vs);
	glCompileShader(sm_fs);

	glAttachShader(mShadowMapProg, sm_vs);
	glAttachShader(mShadowMapProg, sm_fs);
	glLinkProgram(mShadowMapProg);
	glDetachShader(mShadowMapProg, sm_vs);
	glDetachShader(mShadowMapProg, sm_fs);
	glDeleteShader(sm_vs);
	glDeleteShader(sm_fs);
	mShadowMapProgWVPLoc = glGetUniformLocation(mShadowMapProg, "mWVP");

	glUseProgram(mProg);

	mWVPLocation = glGetUniformLocation(mProg, "mWVP");
	mWorldLocation = glGetUniformLocation(mProg, "mWorld");
	mWVPFromLightLocation = glGetUniformLocation(mProg, "mWVPFromLight");
	GLint samplerLocation = glGetUniformLocation(mProg, "mSampler");
	GLint sm_location = glGetUniformLocation(mProg, "mShadowMapSampler");
	glUniform1i(samplerLocation, 0);
	glUniform1i(sm_location, 1);

	vec3 light_position = vec3(100.0f, 100.0f, -100.0f);

	PointLight *point_light = static_cast<PointLight *>(glspGetUniformLocation(mProg, "mPointLight"));
	point_light->Color = vec3(1.0f, 1.0f, 1.0f);
	point_light->Position = light_position;
	point_light->AttenuationConstant = 1.0f;
	point_light->AttenuationLinear = 0.001f;
	point_light->AttenuationSquare = 0.00001f;
	point_light->AmbientIntensity = 0.4f;
	point_light->DiffuseIntensity = 0.6f;
	point_light->SpecularIntensity = 0.6f;

	vec3 *eye_pos = static_cast<vec3 *>(glspGetUniformLocation(mProg, "mEyePos"));
	*eye_pos = vec3(0.0f, 60.0f, 0.0f);

	mCamera.InitCamera(*eye_pos,
					vec3(0.0f, 0.0f, -100.0f),
					vec3(0, 5, -3));

	mTrans   = glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -100.0f));
	mProject = glm::perspective((float)M_PI * 60.0f / 180.0f, 16.0f / 9.0f, 10.0f, 10000.0f);
	mViewProject = mProject * mCamera.GetViewMatrix();

	mat4 quad_scale  = glm::scale(mat4(1.0f), vec3(80.0f, 80.0f, 80.0f));
	mat4 quad_rotate = glm::rotate(mat4(1.0f), (float)M_PI * 90 / 180, vec3(1.0f, 0.0f, 0.0f));
	mQuadTransformWorld = mTrans * quad_rotate * quad_scale;
	mQuadTransformWVP   = mViewProject * mQuadTransformWorld;

	GlspCamera light_view;
	light_view.InitCamera(light_position,
						vec3(20.0f, 0.0f, -100.0f),
						vec3(-5.0f, 4.0f, 0.0f));
	mat4 light_view_project = glm::perspective((float)M_PI * 60.0f / 180.0f, 16.0f / 9.0f, 60.0f, 250.0f);
	mLightVP = light_view_project * light_view.GetViewMatrix();
	mQuadTransformWVPFromLight = mLightVP * mQuadTransformWorld;

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &mShadowMap);
	glBindTexture(GL_TEXTURE_2D, mShadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0);

	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mShadowMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	mQuadTexture = new GlspMaterials(GL_TEXTURE_2D, GLSP_ROOT "assets/test.png");
	mQuadTexture->Load();

	mQuadMesh = new GlspMesh();
	mQuadMesh->LoadMesh(GLSP_ROOT "assets/quad/quad.obj");

	mMesh = new GlspMesh();
	mMesh->LoadMesh(GLSP_ROOT "assets/phoenix/phoenix_ugv.md2");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	return true;
}

void ShadowMap::onRender()
{
	glUseProgram(mShadowMapProg);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUniformMatrix4fv(mShadowMapProgWVPLoc, 1, false, (float *)&mQuadTransformWVPFromLight);
	mQuadMesh->Render();

	mat4 scale  = glm::scale(mat4(1.0f), vec3(0.8f, 0.8f, 0.8f));
	mat4 rotate = glm::rotate(mat4(1.0f), (float)M_PI * mScalar / 180, vec3(0.0f, 1.0f, 0.0f));
	mat4 world  = mTrans * rotate * scale;
	mat4 wvp    = mLightVP * world;
	glUniformMatrix4fv(mShadowMapProgWVPLoc, 1, false, (float *)&wvp);
	mMesh->Render();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(mProg);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(mWVPLocation, 1, false, (float *)&mQuadTransformWVP);
	glUniformMatrix4fv(mWorldLocation, 1, false, (float *)&mQuadTransformWorld);
	glUniformMatrix4fv(mWVPFromLightLocation, 1, false, (float *)&mQuadTransformWVPFromLight);

	mQuadTexture->Bind(GL_TEXTURE0);
	mQuadMesh->Render(true);

	mat4 wvp1 = mViewProject * world;
	glUniformMatrix4fv(mWVPLocation, 1, false, (float *)&wvp1);
	glUniformMatrix4fv(mWorldLocation, 1, false, (float *)&world);
	glUniformMatrix4fv(mWVPFromLightLocation, 1, false, (float *)&wvp);
	mScalar += 1.0f;

	mMesh->Render();
}

void ShadowMap::onKeyPressed(unsigned long key)
{
	if (mCamera.CameraControl(key))
	{
		// This key event has been processed by camera control,
		// need update the TVP matrix.
		mViewProject = mProject * mCamera.GetViewMatrix();
		mQuadTransformWVP = mViewProject * mQuadTransformWorld;
	}
}

void ShadowMap::onMouseLeftClickDown(int x, int y)
{
	// printf("Mouse left click: %d %d", x, y);
}


} // namespace glsp

int main(int argc, char *argv[])
{
	Magick::InitializeMagick(*argv);
	glsp::ShadowMap app;
	app.run();
}