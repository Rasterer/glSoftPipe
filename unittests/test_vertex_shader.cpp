#include <iostream>
#include <string.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GLContext.h"
#include "Shader.h"
#include "glcorearb.h"

using namespace std;
using namespace glm;

Shader *gVS;

class simpleVertexShader: public VertexShader
{
public:
	simpleVertexShader()
	{
		DECLARE_UNIFORM(mView);
		DECLARE_UNIFORM(mProj);
		VS_DECLARE_ATTRIB(vec3, iPos);
		VS_DECLARE_VARYING(vec4, glPosition);
	}

	void execute()
	{
		vec3 iPos[3];
		iPos[0] = vec3(-1.0f, -1.0f, 0.6f);
		iPos[1] = vec3(1.0f, -1.0f, 0.6f);
		iPos[2] = vec3(0.0f, 1.0f, 0.0f);

		for(size_t i = 0; i < 3; i++)
		{
			vsInput in;
			vsOutput out;
			in.assemble(vec4(iPos[i], 0.0f));
			out.outputSize(1);
			onExecute(in, out);
			//cout << __func__ << ": shader output " << out.mOutReg[0].x /out.mOutReg[0].w << endl;
			//cout << __func__ << ": shader output " << out.mOutReg[0].y /out.mOutReg[0].w << endl;
			//cout << __func__ << ": shader output " << out.mOutReg[0].z /out.mOutReg[0].w << endl;
			//cout << __func__ << ": shader output " << out.mOutReg[0].w << endl;
		}
	}

	void onExecute(vsInput &in, vsOutput &out)
	{
		cout << __func__ << ": view is: " << &mView << endl;
		cout << __func__ << ": proj is: " << &mProj << endl;
		//cout << mView[0][0] << endl;
		//cout << mView[1][0] << endl;
		//cout << mView[2][0] << endl;
		//cout << mView[3][0] << endl;
		//cout << mView[0][1] << endl;
		//cout << mView[1][1] << endl;
		//cout << mView[2][1] << endl;
		//cout << mView[3][1] << endl;
		//cout << mView[0][2] << endl;
		//cout << mView[1][2] << endl;
		//cout << mView[2][2] << endl;
		//cout << mView[3][2] << endl;
		//cout << mView[0][3] << endl;
		//cout << mView[1][3] << endl;
		//cout << mView[2][3] << endl;
		//cout << mView[3][3] << endl;
		cout << mProj[0][0] << endl;
		cout << mProj[1][0] << endl;
		cout << mProj[2][0] << endl;
		cout << mProj[3][0] << endl;
		cout << mProj[0][1] << endl;
		cout << mProj[1][1] << endl;
		cout << mProj[2][1] << endl;
		cout << mProj[3][1] << endl;
		cout << mProj[0][2] << endl;
		cout << mProj[1][2] << endl;
		cout << mProj[2][2] << endl;
		cout << mProj[3][2] << endl;
		cout << mProj[0][3] << endl;
		cout << mProj[1][3] << endl;
		cout << mProj[2][3] << endl;
		cout << mProj[3][3] << endl;
		VS_RESOLVE_ATTRIB(vec3, iPos, in);
		VS_RESOLVE_VARYING(vec4, glPosition, out);

		glPosition = mProj * mView * vec4(iPos, 1.0f);
		cout << __func__ << ": shader input z " << iPos.z << endl;
	}
private:
	mat4 mView;
	mat4 mProj;
};

class simpleFragmentShader: public FragmentShader
{
};

class simpleShaderFactory: public ShaderFactory
{
public:
	Shader *createVertexShader()
	{
		gVS = new simpleVertexShader();
		return gVS;
	}

	void DeleteVertexShader(Shader *pVS)
	{
		delete pVS;
	}

	Shader *createFragmentShader()
	{
		return new simpleFragmentShader();
	}

	void DeleteFragmentShader(Shader *pFS)
	{
		delete pFS;
	}
};

int main(void)
{
	ShaderFactory *pFact = new simpleShaderFactory();

	initTLS();
	GLContext *gc = CreateContext();
	MakeCurrent(gc);

	GLuint prog = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

	cout << __func__ << ": before execute" << endl;

	glShaderSource(vs, 0, reinterpret_cast<const char *const*>(pFact), 0);
	glShaderSource(fs, 0, reinterpret_cast<const char *const*>(pFact), 0);

	glCompileShader(vs);
	glCompileShader(fs);

	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glUseProgram(prog);

	GLint viewLocation = glGetUniformLocation(prog, "mView");
	GLint projLocation = glGetUniformLocation(prog, "mProj");

	mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
					   vec3(0.0f, 0.0f, 1.0f),
					   vec3(0.0f, 1.0f, 0.0f));
		//cout << view[0][0] << endl;
		//cout << view[1][0] << endl;
		//cout << view[2][0] << endl;
		//cout << view[3][0] << endl;
		//cout << view[0][1] << endl;
		//cout << view[1][1] << endl;
		//cout << view[2][1] << endl;
		//cout << view[3][1] << endl;
		//cout << view[0][2] << endl;
		//cout << view[1][2] << endl;
		//cout << view[2][2] << endl;
		//cout << view[3][2] << endl;
		//cout << view[0][3] << endl;
		//cout << view[1][3] << endl;
		//cout << view[2][3] << endl;
		//cout << view[3][3] << endl;
	mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 
		cout << project[0][0] << endl;
		cout << project[1][0] << endl;
		cout << project[2][0] << endl;
		cout << project[3][0] << endl;
		cout << project[0][1] << endl;
		cout << project[1][1] << endl;
		cout << project[2][1] << endl;
		cout << project[3][1] << endl;
		cout << project[0][2] << endl;
		cout << project[1][2] << endl;
		cout << project[2][2] << endl;
		cout << project[3][2] << endl;
		cout << project[0][3] << endl;
		cout << project[1][3] << endl;
		cout << project[2][3] << endl;
		cout << project[3][3] << endl;

	cout << __func__ << ": view location " << viewLocation << endl;
	cout << __func__ << ": proj location " << projLocation << endl;
	glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);
	glUniformMatrix4fv(projLocation, 1, false, (float *)&project);

	gVS->execute();
	cout << __func__ << ": after execute" << endl;
	return 0;
}
