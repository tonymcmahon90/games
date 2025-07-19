#include <Windows.h>
#include <Windowsx.h>
#include <math.h>
#include <gl/GL.h>
#pragma comment(lib,"opengl32.lib")
#include <stdio.h>
#include <joystickapi.h> // simple joystick 
#pragma comment(lib,"winmm.lib")

HWND hwnd;
HDC hdc;
HGLRC hglrc;
GLuint base;
float click_x = 0.f, click_y = 0.f; // initial values
double t_diff = 0;
int lifes = 10, score = 0;
bool fullscreen = false, hidecursor = false, usejoystick = true, capturejoystick = true, pausegame = false, sound = true,lclick=false;
char txt[1000];
int grid[10][10],match[3][2],selected=0;
RECT windowrect;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE); // 4k screens that scale to 1080p

	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL,IDC_CROSS),NULL,NULL,L"Match" };
	RegisterClass(&wc);

	hwnd = CreateWindow(L"Match", L"Match", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInst, NULL);
	
	hdc = GetDC(hwnd);

	if (usejoystick) {
		if (joySetCapture(hwnd, JOYSTICKID1, NULL, TRUE)) MessageBox(hwnd, L"No joystick", L"", MB_OK);
		else joySetThreshold(JOYSTICKID1, 10);
	}

	if (hidecursor)ShowCursor(FALSE);

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
	font = CreateFont(-50, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
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

	srand(GetTickCount());

	// grid
	for (int n = 0; n < 10; n++)
		for (int m = 0; m < 10; m++)
			grid[n][m] = rand()%10;
	
	for(int n=0;n<3;n++) 
		match[n][0]= rand() % 10;

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

	if (usejoystick)joyReleaseCapture(JOYSTICKID1);

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
	case WM_CLOSE: PostQuitMessage(0); return 0;
	case WM_SIZE:
		if (!fullscreen) // from windowed to fullscreen 
		{
			GetWindowRect(hwnd, &windowrect);
			MoveWindow(hwnd, windowrect.left, windowrect.top, windowrect.right - windowrect.left, windowrect.bottom - windowrect.top, TRUE);
		}
		glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize.x = 300;
		pMMI->ptMinTrackSize.y = 300;
		return 0;
	}
	case WM_LBUTTONDOWN: lclick = true; return 0;
	case WM_LBUTTONUP: lclick = false; return 0;
	case WM_RBUTTONDOWN: selected = 0; return 0;
	case WM_MOUSEMOVE:
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		int m_x, m_y, c_x, c_y;
		m_x = GET_X_LPARAM(lParam);
		m_y = GET_Y_LPARAM(lParam);
		c_x = rc.right;
		c_y = rc.bottom;

		click_x = (((float)m_x / (float)c_x) * 2.0f) - 1.0f;
		click_y = 1.0f - (((float)m_y / (float)c_y) * 2.0f);
		
		click_x = min(1.f, max(-1.f, click_x));
		click_y = min(1.f, max(-1.f, click_y));	

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
		case 'J': capturejoystick = !capturejoystick; break;
		case 'P': pausegame = !pausegame; break;
		case 'C': if (hidecursor) { ShowCursor(TRUE); hidecursor = false; }
				else { ShowCursor(FALSE); hidecursor = true; }
				break;
		case 'S': sound = !sound; break;
		}
		return 0;
	}
	case MM_JOY1MOVE:
	{
		if (!capturejoystick) return 0;

		int xpos = LOWORD(lParam), ypos = HIWORD(lParam); // 0 to 65535 top left 0,0
		
		click_x = (xpos / 32767.f) - 1.f; // -1.0 left 1 right = 0 to 65535
		click_y = 1.f-(ypos / 32767.f); // 1 top -1 bottom 

		click_x = min(1.f, max(-1.f, click_x));
		click_y = min(1.f, max(-1.f, click_y));

		return 0;
	}
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
	
	int n, m;
	float b_x, b_y;	

	for (n = 0; n < 10; n++)
		for (m = 0; m < 10; m++)
		{
			b_x = -0.8f + (n * 0.11f);
			b_y = 0.8f - (m * 0.11f);

			if (click_x > b_x && click_x < b_x + 0.1f)
				if (click_y > b_y-0.1f && click_y < b_y)
					if (lclick)
					{
						if (grid[n][m] == match[selected][0]) { match[selected][1] = grid[n][m]; selected++; }
					}						
		}

	// graphics 

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);

	glColor3f(0.2f, 0.2f, 0.2f);
	glVertex2f(click_x-0.1f, click_y-0.1f); 
	glVertex2f(click_x+0.1f, click_y-0.1f);
	glVertex2f(click_x+0.1f, click_y+0.1f);
	glVertex2f(click_x-0.1f, click_y+0.1f);

	bool area = false;
	
	for (n = 0; n < 10; n++)
		for (m = 0; m < 10; m++)
		{
			switch (grid[n][m])
			{
			case 0: glColor3f(0.5f, 0, 0.5f); break;
			case 1: glColor3f(1.f, 0, 0); break;
			case 2: glColor3f(0, 1.f, 0); break;
			case 3: glColor3f(0, 0, 1.f); break;
			case 4: glColor3f(1.f, 1.f, 0); break;
			case 5: glColor3f(0.f, 1.f, 1.f); break;
			case 6: glColor3f(1.f, 1.f, 1.f); break;
			case 7: glColor3f(1.f, 0.f, 1.f); break;
			case 8: glColor3f(1.f, 0.5f, 0.5f); break;
			case 9: glColor3f(0.5f, 1.f, 1.f); break;
			}

			b_x = -0.8f + (n *0.11f);
			b_y = 0.8f - (m * 0.11f);

			if (click_x > b_x && click_x < b_x + 0.11f)
				if (click_y > b_y - 0.11f && click_y < b_y)
					area = true;

			if (!area) glColor3f(0.3f, 0.3f, 0.3f);
			area = false;

			if (grid[n][m] != 11)
			{
				glVertex2f(b_x, b_y);
				glVertex2f(b_x + 0.1f, b_y);
				glVertex2f(b_x + 0.1f, b_y - 0.1f);
				glVertex2f(b_x, b_y - 0.1f);
			}
		}

	for(n=0;n<3;n++)
		for (m = 0; m < 2; m++)
		{
			switch (match[n][m])
			{
			case 0: glColor3f(0.5f, 0, 0.5f); break;
			case 1: glColor3f(1.f, 0, 0); break;
			case 2: glColor3f(0, 1.f, 0); break;
			case 3: glColor3f(0, 0, 1.f); break;
			case 4: glColor3f(1.f, 1.f, 0); break;
			case 5: glColor3f(0.f, 1.f, 1.f); break;
			case 6: glColor3f(1.f, 1.f, 1.f); break;
			case 7: glColor3f(1.f, 0.f, 1.f); break;
			case 8: glColor3f(1.f, 0.5f, 0.5f); break;
			case 9: glColor3f(0.5f, 1.f, 1.f); break;
			}

			b_x = 0.5f + (n * 0.11f);
			b_y = 0.8f - (m * 0.11f);
			
			if (match[n][m] != 11)
			{
				glVertex2f(b_x, b_y);
				glVertex2f(b_x + 0.1f, b_y);
				glVertex2f(b_x + 0.1f, b_y - 0.1f);
				glVertex2f(b_x, b_y - 0.1f);
			}
		}

	glEnd();

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2f(-0.25f, 0.85f);
		
	sprintf_s(txt, sizeof(txt), "Select=%d Score=%d", selected, score);
	glPrintf(txt);
	glFlush();
	SwapBuffers(hdc);	

	bool won = true;
	for (n = 0; n < 3; n++)
	{
		if (match[n][0] != match[n][1]) won = false;
	}

	if (won)
	{
		score++; selected = 0;

		for (n = 0; n < 10; n++)
			for (m = 0; m < 10; m++)
				grid[n][m] = rand() % 10;

		for (n = 0; n < 3; n++)
			match[n][0] = rand() % 10;
	}
}