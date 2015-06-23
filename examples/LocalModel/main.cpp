#include <cmath>
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
#include "mesh.h"

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
	mat4 mWVP;
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

	EGLDisplay display = eglGetDisplay(disp);
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

#if 0
	int iIndex[6] = {0, 1, 2, 1, 2, 3};
	//int iIndex[3] = {0, 1, 2};

	struct vertex_color
	{
		vec3 iPos;
		vec2 iTexCoor;
	//} in[6];
	} in[4];

	//in[0].iPos = vec3(-2.0f, -1.0f, 0.6f);
	//in[1].iPos = vec3(1.0f, -1.0f, 0.6f);
	//in[2].iPos = vec3(0.0f, 1.0f, 0.0f);
	in[0].iPos = vec3(-2.0f, -2.0f, 0.0f);
	in[1].iPos = vec3(2.0f, -2.0f, 0.0f);
	in[2].iPos = vec3(-2.0f, 2.0f, 0.0f);
	in[3].iPos = vec3(2.0f, 2.0f, 0.0f);
	//in[3].iPos = vec3(-1.0f, 1.0f, 0.0f);
	//in[4].iPos = vec3(1.0f, 1.0f, 0.0f);
	//in[5].iPos = vec3(0.0f, -1.0f, -1.0f);

	in[0].iTexCoor = vec2(0.0f, 0.0f);
	in[1].iTexCoor = vec2(1.0f, 0.0f);
	in[2].iTexCoor = vec2(0.0f, 1.0f);
	in[3].iTexCoor = vec2(1.0f, 1.0f);
	//in[3].fTexCoor = vec2(255.0f, 255.0f, 0.0f);
	//in[4].fTexCoor = vec2(0.0f, 255.0f, 255.0f);
	//in[5].fTexCoor = vec2(255.0f, 0.0f, 255.0f);

	GLuint bo[2];
	glGenBuffers(2, bo);
	glBindBuffer(GL_ARRAY_BUFFER, bo[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(in), in, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(iIndex), iIndex, GL_STATIC_DRAW);
#endif
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
	glEnable(GL_DEPTH_TEST);

	GLint WVPLocation     = glGetUniformLocation(prog, "mWVP");
	GLint samplerLocation = glGetUniformLocation(prog, "mSampler");

	float xbias = 10.0f;
	mat4 scal  = glm::scale(mat4(1.0f), vec3(0.1f, 0.1f, 0.1f));
	mat4 trans = glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, xbias));
	//rotate = glm::rotate(world, (float)M_PI * 70 / 180, vec3(0.0f, 1.0f, 0.0f));
	//rotate = glm::rotate(world, (float)M_PI * 30 / 180, vec3(1.0f, 0.0f, 0.0f));
	mat4 world = trans * scal;
	mat4 view = glm::lookAt(vec3(15.0f, 10.0f, -10.0f),
					   vec3(0.0f, 0.0f, 10.0f),
					   vec3(0.0f, 1.0f, 0.0f));
	mat4 project = glm::perspective((float)M_PI * 60.0f / 180.0f, 16.0f / 9.0f, 1.0f, 50.0f); 

	mat4 wvp   = project * view * world;

	glUniformMatrix4fv(WVPLocation, 1, false, (float *)&wvp);
	glUniform1i(samplerLocation, 0);
	Magick::InitializeMagick(*argv);
#if 0
	int posLocation   = glGetAttribLocation(prog, "iPos");
	int texCoorLocation = glGetAttribLocation(prog, "iTexCoor");
	glEnableVertexAttribArray(posLocation);
	glEnableVertexAttribArray(texCoorLocation);
	cout << "jzb posLocation " << posLocation << " texCoorLocation " << texCoorLocation << endl;
	glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, sizeof(in[0]), 0);
	glVertexAttribPointer(texCoorLocation, 2, GL_FLOAT, GL_FALSE, sizeof(in[0]), (const void *)sizeof(vec3));

	Magick::InitializeMagick(*argv);
	Magick::Blob blob0;
	Magick::Blob blob1;
	Magick::Blob blob2;
	Magick::Blob blob3;
	Magick::Blob blob4;
	Magick::Blob blob5;
	//Magick::Image* pImage = new Magick::Image("test.png");
	//Magick::Image* pImage0 = new Magick::Image("../examples/materials/test0.tga");
	//Magick::Image* pImage1 = new Magick::Image("../examples/materials/test1.tga");
	//Magick::Image* pImage2 = new Magick::Image("../examples/materials/test2.tga");
	//Magick::Image* pImage3 = new Magick::Image("../examples/materials/test3.tga");
	//Magick::Image* pImage4 = new Magick::Image("../examples/materials/test4.tga");
	//Magick::Image* pImage5 = new Magick::Image("../examples/materials/test5.tga");
	Magick::Image* pImage0 = new Magick::Image("../examples/materials/0.tga");
	Magick::Image* pImage1 = new Magick::Image("../examples/materials/1.tga");
	Magick::Image* pImage2 = new Magick::Image("../examples/materials/2.tga");
	Magick::Image* pImage3 = new Magick::Image("../examples/materials/3.tga");
	Magick::Image* pImage4 = new Magick::Image("../examples/materials/4.tga");
	Magick::Image* pImage5 = new Magick::Image("../examples/materials/5.tga");
	pImage0->write(&blob0, "RGBA");
	pImage1->write(&blob1, "RGBA");
	pImage2->write(&blob2, "RGBA");
	pImage3->write(&blob3, "RGBA");
	pImage4->write(&blob4, "RGBA");
	pImage5->write(&blob5, "RGBA");
	//Magick::Image OffscreenImage(pImage->columns(), pImage->rows(), "RGBA", Magick::CharPixel, blob.data());
	//OffscreenImage.write("output.png");

	//Magick::Image my_image("640x480", "white");
	//my_image.write("output.png");
#endif
	Mesh *pMesh = new Mesh();
	pMesh->LoadMesh("../examples/LocalModel/materials/phoenix_ugv.md2");














#if 0
	GLuint texobj;
	glGenTextures(1, &texobj);
	glBindTexture(GL_TEXTURE_2D, texobj);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pImage0->columns(), pImage0->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob0.data());
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, pImage1->columns(), pImage1->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob1.data());
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, pImage2->columns(), pImage2->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob2.data());
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, pImage3->columns(), pImage3->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob3.data());
	glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA, pImage4->columns(), pImage4->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob4.data());
	glTexImage2D(GL_TEXTURE_2D, 5, GL_RGBA, pImage5->columns(), pImage5->rows(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blob5.data());

//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	//glViewport(640, 360, 640, 360);
#endif
	while(true)
	{
		cout << "draw begin" << endl;

		//xbias += 0.01f;

		//mat4 trans = translate(mat4(1.0f), vec3(xbias, 0.0f, 0.0f));
		//mat4 view = lookAt(vec3(0.0f, 0.0f, -5.0f),
						   //vec3(0.0f, 0.0f, 1.0f),
						   //vec3(0.0f, 1.0f, 0.0f));
		//mat4 project = perspective((float)M_PI * 30.0f / 180.0f, 16.0f / 9.0f, 1.0f, 10.0f); 

		//view = view * trans;
		//glUniformMatrix4fv(viewLocation, 1, false, (float *)&view);

		//glDrawArrays(GL_TRIANGLES, 0, 3);
		pMesh->Render();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		ok = eglSwapBuffers(display, surface);
		std::this_thread::sleep_for (std::chrono::seconds(2));
	}

	cout << __func__ << ": after execute" << endl;

	return 0;
}
