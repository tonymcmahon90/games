#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>
#pragma comment(lib,"opengl32.lib")

HWND hwnd;
HDC hdc;
HGLRC hglrc;
char txt[1000];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
void Init();
void End();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE); // looks better not scaled 

	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL,IDC_ARROW),
		(HBRUSH)GetStockObject(WHITE_BRUSH),NULL,L"OpenGL" };
	RegisterClass(&wc);

	hwnd = CreateWindow(L"OpenGL",L"OpenGL", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
		NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);

	hdc = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,24,
		0,0,0,0,0,0, 0,0,0, 0,0,0,0, 16, 0,0,PFD_MAIN_PLANE,0, 0,0,0 };

	int pixelFormat = ChoosePixelFormat(hdc, &pfd);
	if (SetPixelFormat(hdc, pixelFormat, &pfd))
	{
		hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);
	}
	
	Init();

	MSG msg;

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) break;
		}
		else
			Render();
	}

	End();

	if (hglrc)
	{
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hglrc);
	}
	ReleaseDC(hwnd, hdc);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize.x = 300;
		pMMI->ptMinTrackSize.y = 300;
		return 0;
	}
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

unsigned int VBO,VAO; // vertex buffer object with colors 
float vertices[] = { 
	-0.5f,-0.5f,0, 1.0f,0.0f,0.0f,
	0.5f,-0.5f,0, 0.0f,1.0f,0.0f,
	0,0.5f,0, 0.0f,0.0f,1.0f	 };

unsigned int vertexShader,fragmentShader,shaderProgram;
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 oColor;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"   oColor=aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 oColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(oColor, 1.0f);\n"
"}\n\0";

// glew.h 

#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;

typedef GLuint (APIENTRY * PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY * PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (APIENTRY* PFNGLGENVERTEXARRAYSAPPLEPROC) (GLsizei n, const GLuint* arrays);
typedef void (APIENTRY* PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint* buffers);
typedef void (APIENTRY* PFNGLBINDVERTEXARRAYSGIXPROC) (GLuint array);
typedef void (APIENTRY* PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRY* PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint* arrays);
typedef void (APIENTRY* PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint* buffers);
typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY* PFNGLUSEPROGRAMPROC) (GLuint program);

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLGENVERTEXARRAYSAPPLEPROC glGenVertexArrays;
PFNGLGENBUFFERSARBPROC glGenBuffers;
PFNGLBINDVERTEXARRAYSGIXPROC glBindVertexArray;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLUSEPROGRAMPROC glUseProgram;

void Init()
{	
	// wglGetProc 
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSAPPLEPROC)wglGetProcAddress("glGenVertexArrays");
	glGenBuffers = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffers");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYSGIXPROC)wglGetProcAddress("glBindVertexArray");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	//

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//
		
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // was 3 xyz
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float))); // 3 offset
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}

void End()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);
}


void Render()
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	glViewport(0, 0, rc.right, rc.bottom);
	glClearColor(0.5f, 0.5f, 0.5f, 1);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);

/*	glBegin(GL_TRIANGLES);
	glColor3f(1,0,0);
	glVertex3f(0,0,0);
	glColor3f(1, 1, 0);
	glVertex3f(1,0,0);
	glColor3f(0, 1, 0);
	glVertex3f(0.5f,1,0);
	glEnd(); */

	SwapBuffers(hdc);

	static int fps = 0,sec;
	fps++;
	SYSTEMTIME t;
	GetSystemTime(&t);
	if (t.wSecond != sec)
	{
		sprintf_s(txt, 1000, "OpenGL %d fps", fps);
		SetWindowTextA(hwnd,txt);
		sec = t.wSecond;
		fps = 0;
	}
}
