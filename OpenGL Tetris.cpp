#include <Windows.h>
#include <Windowsx.h>
#include <math.h>
#include <gl/GL.h>
#pragma comment(lib,"opengl32.lib")
#include <stdio.h>

HWND hwnd;
HDC hdc;
HGLRC hglrc;
GLuint base;

double t_diff = 0;
bool fullscreen = false;
char txt[1000];
int scene[10][15],score=0;
bool turn = false, leftshift = false, rightshift = false,fastmode=false;
RECT windowrect;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),NULL,NULL,NULL,L"Tetris" }; // LoadCursor(NULL,IDC_ARROW)
	RegisterClass(&wc);

	int x = GetSystemMetrics(SM_CYFULLSCREEN), x6 = x / 6;

	hwnd = CreateWindow(L"Tetris", L"Tetris", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, x6, x6, x6*4, x6*4,
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

	HFONT font, oldfont;
	base = glGenLists(96);
	font = CreateFont(-100, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Arial");
	oldfont = (HFONT)SelectObject(hdc, font);
	wglUseFontBitmaps(hdc, 32, 96, base);
	SelectObject(hdc, oldfont);
	DeleteObject(font);

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// scene
	for (int n = 0; n < 10; n++)
		for (int m = 0; m < 15; m++)
		{
			scene[n][m] = 0;
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

	if (hglrc)
	{
		glDeleteLists(base, 96);
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
//	case WM_SETCURSOR: if (LOWORD(lParam) == HTCLIENT) { SetCursor(NULL);  return TRUE; } return FALSE;
	case WM_CLOSE: PostQuitMessage(0); return 0;
	case WM_SIZE: 		
		if (!fullscreen) 
		{
			GetWindowRect(hwnd, &windowrect);
			MoveWindow(hwnd, windowrect.left, windowrect.top, windowrect.right - windowrect.left, windowrect.right - windowrect.left, TRUE);
			glViewport(0, 0, LOWORD(lParam), HIWORD(lParam)); return 0;
		}
		else
		{
			glViewport(0, 0, HIWORD(lParam), HIWORD(lParam)); return 0;  // x:y 1:1
		}
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize.x = 300;
		pMMI->ptMinTrackSize.y = 300;	
		return 0;
	}
	
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE: PostQuitMessage(0); break;
		case 'W': fastmode = true; break;
		case 'S': turn = true; break;
		case 'A': leftshift = true; break;
		case 'D': rightshift = true; break;
		case 'F':
		{
			if (!fullscreen) // go to full screen
			{ 
				fullscreen = true; 
				SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS); ShowWindow(hwnd, SW_SHOWMAXIMIZED);  
				break; 
			}
			else // from full screen to windowed 
			{
				fullscreen = false;
				MoveWindow(hwnd, windowrect.left, windowrect.top, windowrect.right-windowrect.left, windowrect.bottom-windowrect.top,TRUE);
				SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
				ShowWindow(hwnd, SW_NORMAL);
				break;
			}
		}
		}
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void glPrintf(const char* fmt, ...)
{
	char text[256];
	va_list ap;
	if (fmt == NULL)return;
	va_start(ap, fmt);
	vsprintf_s(text, sizeof(text), fmt, ap);
	va_end(ap);

	glPushAttrib(GL_LIST_BASE);
	glListBase(base - 32);
	glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
	glPopAttrib();
}

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

	// game logic
	
	static double timer;
	timer += tmp;

	if (timer > 0.2 || fastmode)
	{
		bool dropping = false;
		timer = 0;
		fastmode = false;

		// check if a row is full and delete
		bool full = true;
		for (int m =14; m>0; m--)
		{
			for (int n = 0; n < 10; n++)
			{
				if(scene[n][m] == 0) full = false;				
			}

			if (full)
			{
				for(int n = 0; n < 10; n++) scene[n][m] = 0;
				score++;
			}

			full = true;
		}

		// drop tiles
		for(int n=0;n<10;n++)
			for (int m = 13; m>=0; m--)
			{		
				if (scene[n][m + 1] == 0 && scene[n][m] != 0)
				{
					bool moved = false;		
					dropping = true;

					if (leftshift)
					{
						if (scene[n-1][m + 1] == 0 && scene[n][m] != 0)
						if (n > 0) // left wall
						{
							scene[n-1][m + 1] = scene[n][m];
							if (m == 0)scene[n][m] = 0; else scene[n][m] = scene[n][m - 1];	
							moved = true;
						}						
						leftshift = false;						
					}			

					if (rightshift)
					{
						if (scene[n+1][m + 1] == 0 && scene[n][m] != 0)
						if (n < 9)
						{
							scene[n + 1][m + 1] = scene[n][m];
							if (m == 0)scene[n][m] = 0; else scene[n][m] = scene[n][m - 1];	
							moved = true;
						}							
						rightshift = false;
						
					}
					
					if(!moved)
					{						
						scene[n][m + 1] = scene[n][m];
						if (m == 0)scene[n][m] = 0; else scene[n][m] = scene[n][m - 1];
						
					}					
				}
			}

		// add new tiles
		if(dropping==false) scene[rand() % 10][0] = rand() % 5;
	}

	// graphics 

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);

	float s_x, s_y;
		
	for (int n = 0; n < 10; n++)
		for (int m = 0; m < 15; m++)
		{
			switch (scene[n][m])
			{
			case 1: glColor3f(1.f, 0, 0); break;
			case 2: glColor3f(0, 1.f, 0); break;
			case 3: glColor3f(0, 0, 1.f); break;
			case 4: glColor3f(1.f, 1.f, 0); break;
			case 5: glColor3f(1.f, 1.f, 1.f); break;
			}

			s_x = -0.5f + (n / 10.0f);
			s_y = 0.9f - (m / 10.f);

			if (scene[n][m] != 0)
			{
				glVertex2f(s_x, s_y);
				glVertex2f(s_x + 0.1f, s_y);
				glVertex2f(s_x + 0.1f, s_y - 0.1f);
				glVertex2f(s_x, s_y - 0.1f);
			}
		}

	glEnd();  

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2f(-1.f, 0.75f);

	//sprintf_s(txt, sizeof(txt), "Qt=%ld %ld %f", qt_diff.QuadPart,qt_freq.QuadPart,t_diff);
	sprintf_s(txt, sizeof(txt), "%d", score);
	glPrintf(txt);
	glFlush();
	SwapBuffers(hdc);
}