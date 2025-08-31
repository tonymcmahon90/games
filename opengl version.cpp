#include <Windows.h>
#include <gl/GL.h>
#pragma comment(lib,"opengl32.lib")
#include <stdio.h>

HWND hwnd = NULL;
HDC hdc = NULL;
HGLRC hglrc = NULL;
char txt[10000];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL,IDC_HAND),(HBRUSH)COLOR_BACKGROUND+1,NULL,L"OpenGL"}; // LoadCursor(NULL,IDC_ARROW)
	RegisterClass(&wc);	

	hwnd = CreateWindow(L"OpenGL", L"OpenGL", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1000,1000,
		NULL, NULL, hInst, NULL);
	
	hdc = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1,PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,24,
		0,0,0,0,0,0, 0,0,0, 0,0,0,0, 16, 0,0,PFD_MAIN_PLANE,0, 0,0,0 };

	int pFormat = ChoosePixelFormat(hdc, &pfd);
	if (SetPixelFormat(hdc, pFormat, &pfd))
	{
		hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);
	}
	
	ShowWindow(hwnd, SW_SHOW);
	MSG msg;

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else Render();
	}

	if(hglrc)
	{		
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hglrc);
	}
	ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_KEYDOWN:		
		sprintf_s(txt,10000, "%s\n%s\n%s\n%s\n",(char*)glGetString(GL_VERSION), (char*)glGetString(GL_VENDOR), (char*)glGetString(GL_RENDERER), (char*)glGetString(GL_EXTENSIONS));
		MessageBoxA(NULL, txt, "", MB_OK);
		return 0;
	case WM_CLOSE: 
		PostQuitMessage(0); 
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Render()
{
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	static float angle = 0;
	glRotatef(angle+=0.1f, 0, 0, 1);
	glBegin(GL_QUADS);
	glColor3f(1, 0, 0);
	glVertex2f(0, 0);
	glColor3f(0, 1, 1);
	glVertex2f(0.5f, 0);
	glColor3f(0, 1, 0);
	glVertex2f(0.5f, 0.5f);
	glColor3f(1, 1, 1);
	glVertex2f(0, 0.5f);
	glEnd();
	glFlush();
	SwapBuffers(hdc);
}
