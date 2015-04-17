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
Shader *gFS;

class simpleVertexShader: public VertexShader
{
public:
	simpleVertexShader()
	{
		DECLARE_UNIFORM(mView);
		DECLARE_UNIFORM(mProj);
		DECLARE_IN (vec3, iPos);
		DECLARE_OUT(vec4, glPosition);
	}

	void execute(vsInput &in, vsOutput &out)
	{
		cout << "jzb: before VS" << endl;
		RESOLVE_IN (vec3, iPos, in);
		RESOLVE_OUT(vec4, glPosition, out);
		cout << "iPos x " << glPosition.x << endl;
		cout << "iPos y " << glPosition.y << endl;
		cout << "iPos z " << glPosition.z << endl;

		glPosition = mProj * mView * vec4(iPos, 1.0f);
		cout << "jzb: after VS" << endl;
		cout << "glPosition x " << glPosition.x << endl;
		cout << "glPosition y " << glPosition.y << endl;
		cout << "glPosition z " << glPosition.z << endl;
		cout << "glPosition w " << glPosition.w << endl;
	}

private:
	mat4 mView;
	mat4 mProj;
};

class simpleFragmentShader: public FragmentShader
{
public:
	simpleFragmentShader()
	{
		DECLARE_IN(vec4, glPosition);
		DECLARE_OUT(vec4, FragColor);
	}
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
		gFS = new simpleFragmentShader();
		return gFS;
	}

	void DeleteFragmentShader(Shader *pFS)
	{
		delete gFS;
	}
};

int main(void)
{
	ShaderFactory *pFact = new simpleShaderFactory();

	initTLS();
	GLContext *gc = CreateContext();
	MakeCurrent(gc);

	int iIndex[3] = {0, 1, 2};
	vec3 iPos[3];
	iPos[0] = vec3(-1.0f, -1.0f, 0.6f);
	iPos[1] = vec3(1.0f, -1.0f, 0.6f);
	iPos[2] = vec3(0.0f, 1.0f, 0.0f);

	GLuint bo[2];
	glGenBuffers(2, bo);
	glBindBuffer(GL_ARRAY_BUFFER, bo[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(iPos), iPos, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(iIndex), iIndex, GL_STATIC_DRAW);

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
	mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 

	cout << __func__ << ": view location " << viewLocation << endl;
	cout << __func__ << ": proj location " << projLocation << endl;
	glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);
	glUniformMatrix4fv(projLocation, 1, false, (float *)&project);

	cout << __func__ << ": before execute 0" << endl;
	int posLocation = glGetAttribLocation(prog, "iPos");
	cout << __func__ << ": before execute 1" << endl;
	glEnableVertexAttribArray(posLocation);
	cout << __func__ << ": before execute 2" << endl;
	glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
	cout << __func__ << ": before execute 3" << endl;

	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

	cout << __func__ << ": after execute" << endl;

	MakeCurrent(NULL);
	deinitTLS();

	delete pFact;
	return 0;
}
