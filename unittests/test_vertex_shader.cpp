#include <iostream>
#include <string.h>
#include <thread>
#include <chrono>

#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Magick++.h>

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
		DECLARE_IN(vec2, iTexCoor);

		DECLARE_OUT(vec4, gl_Position);
		DECLARE_OUT(vec2, oTexCoor);
	}

	void execute(vsInput &in, vsOutput &out)
	{
		//cout << "jzb: before VS" << endl;
		RESOLVE_IN (vec3, iPos, in);
		RESOLVE_IN (vec2, iTexCoor, in);

		RESOLVE_OUT(vec4, gl_Position, out);
		RESOLVE_OUT(vec2, oTexCoor, out);

		cout << "iPos x " << iPos.x << endl;
		cout << "iPos y " << iPos.y << endl;
		cout << "iPos z " << iPos.z << endl;

		gl_Position = mProj * mView * vec4(iPos, 1.0f);
		cout << "gl_color x " << iTexCoor.x << endl;
		cout << "gl_color y " << iTexCoor.y << endl;
		oTexCoor = iTexCoor;
		//cout << "jzb: after VS" << endl;
		cout << "gl_Position x " << gl_Position.x << endl;
		cout << "gl_Position y " << gl_Position.y << endl;
		cout << "gl_Position z " << gl_Position.z << endl;
		cout << "gl_Position w " << gl_Position.w << endl;
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
		DECLARE_SAMPLER(mSampler);

		DECLARE_IN(vec4, gl_Position);
		DECLARE_IN(vec2, oTexCoor);

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

private:
	sampler2D mSampler;
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

int main(int argc, char **argv)
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

	//int iIndex[6] = {0, 1, 2, 3, 4, 5};
	int iIndex[3] = {0, 1, 2};

	struct vertex_color
	{
		vec3 iPos;
		vec2 iTexCoor;
	//} in[6];
	} in[3];

	//in[0].iPos = vec3(-2.0f, -1.0f, 0.6f);
	//in[1].iPos = vec3(1.0f, -1.0f, 0.6f);
	//in[2].iPos = vec3(0.0f, 1.0f, 0.0f);

	in[0].iPos = vec3(-1.0f, -1.0f, 0.0f);
	in[1].iPos = vec3(1.0f, -1.0f, 0.0f);
	in[2].iPos = vec3(0.0f, 1.0f, -1.0f);
	//in[3].iPos = vec3(-1.0f, 1.0f, 0.0f);
	//in[4].iPos = vec3(1.0f, 1.0f, 0.0f);
	//in[5].iPos = vec3(0.0f, -1.0f, -1.0f);

	in[0].iTexCoor = vec2(1.0f, 0.0f);
	in[1].iTexCoor = vec2(0.0f, 0.0f);
	in[2].iTexCoor = vec2(0.5f, 1.0f);
	//in[3].fTexCoor = vec2(255.0f, 255.0f, 0.0f);
	//in[4].fTexCoor = vec2(0.0f, 255.0f, 255.0f);
	//in[5].fTexCoor = vec2(255.0f, 0.0f, 255.0f);

	GLuint bo[2];
	glGenBuffers(2, bo);
	glBindBuffer(GL_ARRAY_BUFFER, bo[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(in), in, GL_STATIC_DRAW);
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

	GLint viewLocation    = glGetUniformLocation(prog, "mView");
	GLint projLocation    = glGetUniformLocation(prog, "mProj");
	GLint samplerLocation = glGetUniformLocation(prog, "mSampler");

	float xbias = 0.1f;
	//mat4 trans = translate(mat4(1.0f), vec3(xbias, 0.0f, 0.0f));
	mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
					   vec3(0.0f, 0.0f, 1.0f),
					   vec3(0.0f, 1.0f, 0.0f));
	mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 

	glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);
	glUniformMatrix4fv(projLocation, 1, false, (float *)&project);
	glUniform1i(samplerLocation, 0);

	int posLocation   = glGetAttribLocation(prog, "iPos");
	int texCoorLocation = glGetAttribLocation(prog, "iTexCoor");
	glEnableVertexAttribArray(posLocation);
	glEnableVertexAttribArray(texCoorLocation);
	glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, sizeof(in[0]), 0);
	glVertexAttribPointer(texCoorLocation, 2, GL_FLOAT, GL_FALSE, sizeof(in[0]), (const void *)sizeof(vec3));


	Magick::InitializeMagick(*argv);
	Magick::Blob blob;
	//Magick::Image* pImage = new Magick::Image("test.png");
	Magick::Image* pImage = new Magick::Image("test.png");
	pImage->write(&blob, "RGBA");
	Magick::Image OffscreenImage(pImage->columns(), pImage->rows(), "RGBA", Magick::CharPixel, blob.data());
	//OffscreenImage.write("output.png");

	//Magick::Image my_image("640x480", "white");
	//my_image.write("output.png");

	GLuint texobj;
	glGenTextures(1, &texobj);
	glBindTexture(GL_TEXTURE_2D, texobj);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pImage->columns(), pImage->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glViewport(640, 360, 640, 360);
	while(true)
	{
		cout << "draw begin" << endl;
		std::this_thread::sleep_for (std::chrono::seconds(2));

		//xbias += 0.01f;

		//mat4 trans = translate(mat4(1.0f), vec3(xbias, 0.0f, 0.0f));
		//mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
						   //vec3(0.0f, 0.0f, 1.0f),
						   //vec3(0.0f, 1.0f, 0.0f));
		//mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 

		//view = view * trans;
		//glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		ok = eglSwapBuffers(display, surface);
	}

	cout << __func__ << ": after execute" << endl;

	return 0;
}
