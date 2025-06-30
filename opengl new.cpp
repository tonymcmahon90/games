// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)    https://www.gamedev.net/reference/articles/article1929.asp    https://learnopengl.com/Getting-started/Hello-Triangle

// youtube https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2&index=1 Cherno 

#define GLEW_STATIC 
#include <windows.h>
#include <stdio.h>
#include "/glew-2.1.0/include/GL/glew.h" // https://www.opengl.org/sdk/libs/GLEW/ before GL.h
#pragma comment(lib,"/glew-2.1.0/lib/Release/x64/glew32s.lib") // https://stackoverflow.com/questions/28595833/glew-i-set-it-to-use-the-static-library-but-at-run-time-complains-about-not-fin 
#include <gl/GL.h>
#pragma comment(lib,"opengl32.lib")

HWND hwnd;
HDC hdc;
HGLRC hglrc;

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

	// char t[100];	sprintf_s(t, 100,"%s", glGetString(GL_VERSION));		MessageBoxA(hwnd,t, NULL, NULL);

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

unsigned int VBO,VAO; // vertex buffer object 
float vertices[] = { -0.5f,-0.5f,0, 0.5f,-0.5f,0, 0,0.5f,0 };

unsigned int vertexShader,fragmentShader,shaderProgram;
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.0f, 0.9f, 0.2f, 1.0f);\n"
"}\n\0";

void Init()
{	
	GLenum err = glewInit();  // https://stackoverflow.com/questions/12329082/glcreateshader-is-crashing    must be after create context 

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

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

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
	glClearColor(0.0f, 0.0f, 1, 1);
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
}
