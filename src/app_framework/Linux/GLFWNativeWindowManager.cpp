#include "INativeWindowManager.h"

#include <string>
#include <cassert>
#include <cstdio>
#include <cstring>

#include <GL/glew.h>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include "IAppFramework.h"
#include "glsp_debug.h"


namespace glsp {

static void GLFWErrorCallback(int error, const char* description)
{
	printf("GLFW error %d - %s\n", error, description);
}

class GLFWNativeWindowManager: public INativeWindowManager
{
public:
	GLFWNativeWindowManager();
	virtual ~GLFWNativeWindowManager();

	virtual bool NWMCreateWindow(int w, int h, const char *name);
	virtual void NWMDestroyWindow();
    virtual void EnterLoop();
	virtual void GetWindowInfo(NWMWindowInfo *win_info);
	virtual bool DisplayFrame(NWMBufferToDisplay *buf);

private:
	void CalcFPS();

	GLFWwindow    *mWND;

	GLuint         mVAO;
	GLuint         mVBO;
	GLuint         mProg;
	GLuint         mColorMap;
};


static const char vs_source[] =
	"#version 330\n"
	"layout (location = 0) in vec2 Position;\n"
	"out vec2 TexCoord;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vec4(Position, 0.0f ,1.0f);\n"
	"	TexCoord    = Position * 0.5f + 0.5f;\n"
	"}\n";

static const char fs_source[] =
	"#version 330\n"
	"in vec2 TexCoord;\n"
	"out vec4 FragColor;\n"
	"uniform sampler2D gColorMap;\n"
	"void main()\n"
	"{ \n"
	"	FragColor = texture(gColorMap, TexCoord); \n"
	"} \n";


static long GLFWKeyToGLSPKey(int key)
{
	switch (key)
	{
		case GLFW_KEY_W:
			return 'W';
		case GLFW_KEY_A:
			return 'A';
		case GLFW_KEY_S:
			return 'S';
		case GLFW_KEY_D:
			return 'D';
		case GLFW_KEY_Z:
			return 'Z';
		case GLFW_KEY_X:
			return 'X';
		case GLFW_KEY_U:
			return 'U';
		case GLFW_KEY_H:
			return 'H';
		case GLFW_KEY_J:
			return 'J';
		case GLFW_KEY_K:
			return 'K';
		case GLFW_KEY_M:
			return 'M';
		default:
			//printf("Unknown GLSP key\n");
			return -1;
	}
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//if (action == GLFW_PRESS && GLFWKeyToGLSPKey(key) != -1)
	if (GLFWKeyToGLSPKey(key) != -1)
	{
		INativeWindowManager::get()->GetAppFramework()->onKeyPressed(key);
	}
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		::glfwGetCursorPos(window, &xpos, &ypos);
		INativeWindowManager::get()->GetAppFramework()->onMouseLeftClickDown((int)xpos, (int)ypos);
	}
}

GLFWNativeWindowManager::GLFWNativeWindowManager():
	mWND(nullptr),
	mVAO(0),
	mVBO(0),
	mProg(0),
	mColorMap(0)
{
}

GLFWNativeWindowManager::~GLFWNativeWindowManager()
{
	NWMDestroyWindow();
}

bool GLFWNativeWindowManager::NWMCreateWindow(int w, int h, const char *name)
{
	GLint location;
	GLuint vs;
	GLuint fs;
	GLenum res;
	GLint success;
	GLchar log[1024];
	const GLchar *vs_ptr = vs_source;
	const GLchar *fs_ptr = fs_source;
	const GLint vs_length = sizeof(vs_source);
	const GLint fs_length = sizeof(fs_source);

	struct Point2
	{
		float x;
		float y;
	} vert_buf[4] = {{-1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, -1.0f}, {1.0f, -1.0f}};

	INativeWindowManager::NWMCreateWindow(w, h, name);

	::glfwSetErrorCallback(GLFWErrorCallback);

	if (!::glfwInit())
	{
		printf("GLFW init failed!\n");
		goto err;
	}

	int major, minor, rev;
	::glfwGetVersion(&major, &minor, &rev);
	printf("GLFW %d.%d.%d initialized\n", major, minor, rev);

	::glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	::glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	::glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	mWND = ::glfwCreateWindow(mWNDWidth, mWNDHeight, mWNDName, nullptr, nullptr);
	if (!mWND)
	{
		printf("GLFW create window failed!\n");
		goto terminate;
	}
	::glfwSetKeyCallback(mWND, KeyCallback);
	::glfwSetMouseButtonCallback(mWND, MouseButtonCallback);

	::glfwMakeContextCurrent(mWND);

	glewExperimental = GL_TRUE;
	res = glewInit();
	if (res != GLEW_OK)
	{
		printf("GLEW init failed\n");
		goto destroy_window;
	}

	::glGenVertexArrays(1, &mVAO);
	::glBindVertexArray(mVAO);

	::glGenBuffers(1, &mVBO);
	::glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	::glBufferData(GL_ARRAY_BUFFER, sizeof(vert_buf), &vert_buf[0], GL_STATIC_DRAW);

    ::glEnableVertexAttribArray(0);
    ::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	mProg = ::glCreateProgram();
	vs    = ::glCreateShader(GL_VERTEX_SHADER);
	fs    = ::glCreateShader(GL_FRAGMENT_SHADER);

	::glShaderSource(vs, 1, &vs_ptr, &vs_length);
	::glCompileShader(vs);
	::glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		::glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
		printf("Compile vertex shader failed: %s\n", log);
		goto delete_vbo;
	}
	::glAttachShader(mProg, vs);

	::glShaderSource(fs, 1, &fs_ptr, &fs_length);
	::glCompileShader(fs);
	::glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		::glGetShaderInfoLog(fs, sizeof(log), nullptr, log);
		printf("Compile fragment shader failed: %s\n", log);
		goto delete_vbo;
	}
	::glAttachShader(mProg, fs);

	::glLinkProgram(mProg);
	::glGetProgramiv(mProg, GL_LINK_STATUS, &success);
	if (!success)
	{
		::glGetProgramInfoLog(mProg, sizeof(log), nullptr, log);
		printf("Link program failed: %s\n", log);
		goto delete_vbo;
	}

	::glValidateProgram(mProg);
	::glGetProgramiv(mProg, GL_VALIDATE_STATUS, &success);
	if (!success)
	{
		::glGetProgramInfoLog(mProg, sizeof(log), nullptr, log);
		printf("Validate program failed: %s\n", log);
		goto delete_vbo;
	}

	::glDeleteShader(vs);
	::glDeleteShader(fs);
	::glUseProgram(mProg);

	location = ::glGetUniformLocation(mProg, "gColorMap");
	if (location == -1)
	{
		printf("Failed to get uniform <gColorMap> location\n");
		goto delete_vbo;
	}
	::glUniform1i(location, 0);

	::glGenTextures(1, &mColorMap);
	::glBindTexture(GL_TEXTURE_2D, mColorMap);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	::glViewport(0, 0, mWNDWidth, mWNDHeight);

	// Not aligned with vsync.
	::glfwSwapInterval(0);

	return true;

delete_vbo:
	if (fs)
		::glDeleteShader(fs);
	if (vs)
		::glDeleteShader(vs);
	if (mProg)
	{
		::glDeleteProgram(mProg);
		mProg = 0;
	}

	if (mVBO)
	{
		::glDeleteBuffers(1, &mVBO);
		mVBO = 0;
	}
	if (mVAO)
	{
		::glDeleteVertexArrays(1, &mVAO);
		mVAO = 0;
	}

destroy_window:
	::glfwDestroyWindow(mWND);
	mWND = nullptr;

terminate:
	::glfwTerminate();

err:
	assert(false);

	return false;
}

void GLFWNativeWindowManager::NWMDestroyWindow()
{
	if (mColorMap)
	{
		::glDeleteTextures(1, &mColorMap);
		mColorMap = 0;
	}
	if (mProg)
	{
		::glDeleteProgram(mProg);
		mProg = 0;
	}
	if (mVBO)
	{
		::glDeleteBuffers(1, &mVBO);
		mVBO = 0;
	}
	if (mVAO)
	{
		::glDeleteVertexArrays(1, &mVAO);
		mVAO = 0;
	}

	if (mWND)
	{
		::glfwDestroyWindow(mWND);
		mWND = nullptr;
	}

	::glfwTerminate();
}

void GLFWNativeWindowManager::EnterLoop()
{
    // Main message loop
	IAppFramework *app_framework = GetAppFramework();

	while (!::glfwWindowShouldClose(mWND))
	{
		::glfwPollEvents();
		app_framework->Render();
	}
}

void GLFWNativeWindowManager::GetWindowInfo(NWMWindowInfo *win_info)
{
	win_info->width  = mWNDWidth;
	win_info->height = mWNDHeight;
	//win_info->format = RGBA;
}

bool GLFWNativeWindowManager::DisplayFrame(NWMBufferToDisplay *buf)
{
	// TODO: check format as well
	if (buf->width != mWNDWidth || buf->height != mWNDHeight)
		GLSP_DPF(GLSP_DPF_LEVEL_WARNING, "buffer size mismatch\n");

    ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buf->width, buf->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf->addr);
	::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	::glfwSwapBuffers(mWND);

	CalcFPS();

	return true;
}

void GLFWNativeWindowManager::CalcFPS()
{
	static int sFrameCount = 0;
	static double sStartTime = ::glfwGetTime();

	const double elapsed_time = ::glfwGetTime() - sStartTime;

	if (elapsed_time > 5.0)
	{
		const double fps = (sFrameCount + 1) / elapsed_time;
		std::string wnd_title(mWNDName);

		wnd_title += std::string(" | FPS: ");
		wnd_title += std::to_string(fps);
		::glfwSetWindowTitle(mWND, wnd_title.c_str());

		sFrameCount = 0;
		sStartTime = ::glfwGetTime();
	}
	else
	{
		sFrameCount++;
	}
}

INativeWindowManager* CreateNativeWindowManager()
{
	return new GLFWNativeWindowManager();
}

} // namespace glsp
