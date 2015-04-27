#include <iostream>
#include <string.h>
#include <thread>
#include <chrono>

#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include "khronos/GL/glcorearb.h"
#include "khronos/EGL/egl.h"

using namespace std;
using namespace glm;
using namespace glsp::ogl;

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
		//cout << "jzb: before VS" << endl;
		RESOLVE_IN (vec3, iPos, in);
		RESOLVE_OUT(vec4, glPosition, out);
		//cout << "iPos x " << glPosition.x << endl;
		//cout << "iPos y " << glPosition.y << endl;
		//cout << "iPos z " << glPosition.z << endl;

		glPosition = mProj * mView * vec4(iPos, 1.0f);
		//cout << "jzb: after VS" << endl;
		//cout << "glPosition x " << glPosition.x << endl;
		//cout << "glPosition y " << glPosition.y << endl;
		//cout << "glPosition z " << glPosition.z << endl;
		//cout << "glPosition w " << glPosition.w << endl;
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

void setupXenv()
{
}

int main(void)
{
	xcb_generic_error_t* error;

	Display* disp = XOpenDisplay(NULL);
	if(!disp)
		assert(false);

	xcb_connection_t *connection = XGetXCBConnection(disp);
	if (!connection)
		assert(false);
	if (xcb_connection_has_error(connection))
		assert(false);

	const xcb_setup_t* setup = xcb_get_setup(connection);
	xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;
	assert(screen != 0);

	xcb_window_t window = xcb_generate_id(connection);
	assert(window > 0);

	xcb_void_cookie_t create_cookie = xcb_create_window_checked(
			connection,
			XCB_COPY_FROM_PARENT,
			window,
			screen->root,
			0, 0, 1280, 720,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			0, NULL);

	xcb_void_cookie_t map_cookie = xcb_map_window_checked(connection, window);
	error = xcb_request_check(connection, create_cookie);
	if (error)
		assert(false);

	error = xcb_request_check(connection, map_cookie);
	if (error)
		assert(false);

	EGLint ignore;
	EGLBoolean ok;

	cout << "jzb: main start" << endl;
	EGLDisplay display = eglGetDisplay(disp);
	cout << "jzb: main start 1" << endl;
	ok = eglInitialize(display, &ignore, &ignore);

	if (!ok)
		assert(false);

	EGLContext context = eglCreateContext(
			display,
			NULL,
			EGL_NO_CONTEXT,
			NULL);

	if (!context)
		assert(false);

	EGLSurface surface = eglCreateWindowSurface(
			display,
			NULL,
			window,
			NULL);

	ok = eglMakeCurrent(display, surface, surface, context);

	if (!ok)
		assert(false);

	ShaderFactory *pFact = new simpleShaderFactory();

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

	float xbias = 0.1f;
	mat4 trans = translate(mat4(1.0f), vec3(xbias, 0.0f, 0.0f));
	mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
					   vec3(0.0f, 0.0f, 1.0f),
					   vec3(0.0f, 1.0f, 0.0f));
	mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 

	view = view * trans;
	glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);
	glUniformMatrix4fv(projLocation, 1, false, (float *)&project);

	int posLocation = glGetAttribLocation(prog, "iPos");
	glEnableVertexAttribArray(posLocation);
	glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//glViewport(640, 360, 1280, 720);
	while(true)
	{
		//glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
		std::this_thread::sleep_for (std::chrono::seconds(1));

		xbias += 0.01f;

		mat4 trans = translate(mat4(1.0f), vec3(xbias, 0.0f, 0.0f));
		mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
						   vec3(0.0f, 0.0f, 1.0f),
						   vec3(0.0f, 1.0f, 0.0f));
		mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 

		view = view * trans;
		glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		ok = eglSwapBuffers(display, surface);
	}

	cout << __func__ << ": after execute" << endl;

	return 0;
}
