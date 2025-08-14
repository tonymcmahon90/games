#include <Windows.h>
#include <math.h>
#include <gl/GL.h>
#pragma comment(lib,"opengl32.lib")
#include <stdio.h>

HWND hwnd;
HDC hdc;
HGLRC hglrc;
double t_diff = 0;
bool fullscreen = false, hidecursor = false;
char txt[1000];
RECT windowrect;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE); // 4k screens that scale to 1080p

	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL,IDC_ARROW),NULL,NULL,L"Bitmap" };
	RegisterClass(&wc);

	hwnd = CreateWindow(L"Bitmap", L"Bitmap", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInst, NULL);

	if (hidecursor)ShowCursor(FALSE);

	hdc = GetDC(hwnd);	

	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1,PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,24,
		0,0,0,0,0,0, 0,0,0, 0,0,0,0, 16, 0,0,PFD_MAIN_PLANE,0, 0,0,0 };

	int pFormat = ChoosePixelFormat(hdc, &pfd);
	if (SetPixelFormat(hdc, pFormat, &pfd))
	{
		hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);
	}	

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	ShowWindow(hwnd, show);

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

	if (hglrc)
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
	case WM_CLOSE: PostQuitMessage(0); return 0;
	case WM_SIZE:
		if (!fullscreen) // from windowed to fullscreen 
		{
			GetWindowRect(hwnd, &windowrect);
			MoveWindow(hwnd, windowrect.left, windowrect.top, windowrect.right - windowrect.left, windowrect.bottom - windowrect.top, TRUE);
		}
	//	glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize.x = 300;
		pMMI->ptMinTrackSize.y = 300;
		return 0;
	}	
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_ESCAPE: PostQuitMessage(0); break;
		case 'F':
		{
			if (!fullscreen) // go to full screen
			{
				fullscreen = true;
				SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
				ShowWindow(hwnd, SW_SHOWMAXIMIZED);
			}
			else // from full screen to windowed ( need to fix for multi-monitor ) 
			{
				fullscreen = false;
				MoveWindow(hwnd, windowrect.left, windowrect.top, windowrect.right - windowrect.left, windowrect.bottom - windowrect.top, TRUE);
				SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
				ShowWindow(hwnd, SW_NORMAL);
			}
			break;
		}
	
		case 'C': if (hidecursor) { ShowCursor(TRUE); hidecursor = false; }
				else { ShowCursor(FALSE); hidecursor = true; }
				break;		
		}
		return 0;
	}	
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

GLubyte rasters[24] = { 0xc0,0,0xc0,0,0xc0,0,0xc0,0,0xc0,0,
					  0xff,0,0xff,0,0xc0,0,0xc0,0,0xc0,0,
					  0xff,0xc0,0x70,0xc0 };

void Render()
{
	if ((GetWindowLong(hwnd, GWL_STYLE) & WS_MINIMIZE)) return; // don't render if minimized

	static LARGE_INTEGER qt, qt_prev, qt_diff, qt_freq;
	QueryPerformanceFrequency(&qt_freq);
	if (qt_prev.QuadPart == 0) QueryPerformanceCounter(&qt_prev);
	QueryPerformanceCounter(&qt);
	qt_diff.QuadPart = qt.QuadPart - qt_prev.QuadPart;
	double tmp = (double)qt_diff.QuadPart / (double)qt_freq.QuadPart;
	if (tmp < (1.0 / 60.0)) return; // No more than 60fps

	t_diff = tmp;
	qt_prev.QuadPart = qt.QuadPart;	

	// graphics 

	RECT client;
	GetClientRect(hwnd, &client);
	glViewport(0, 0, client.right, client.bottom);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, client.right, 0, client.bottom, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

	glBegin(GL_QUADS);
	glColor3f(1, 0, 0);
	glVertex2d(50, 50);
	glColor3f(1, 1, 0);
	glVertex2d(100, 50);
	glColor3f(1, 0, 1);
	glVertex2d(100, 100);
	glColor3f(0, 1, 1);
	glVertex2d(50, 100);
	glEnd();

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(20, 20);
	for(int n=0;n<100;n++)
		glBitmap(10, 12, 0, 0, 11, 0, rasters);
	
	glFlush();
	SwapBuffers(hdc);
}