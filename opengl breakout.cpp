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
float bat_x = -1.f, bat_y = -0.9f, ball_x = 0, ball_y = 0, ball_speed = 0.7f;
double t_diff = 0;
int lifes = 10;
bool fullscreen = false;
char txt[1000];
int bricks[10][5];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),NULL,NULL,NULL,L"Breakout"}; // LoadCursor(NULL,IDC_ARROW)
	RegisterClass(&wc);

	hwnd = CreateWindow(L"Breakout", L"Breakout", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

	// bricks
	for(int n=0;n<10;n++)
		for (int m = 0; m < 5; m++)
		{
			bricks[n][m] = 5-m;
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
	case WM_SETCURSOR: if (LOWORD(lParam) == HTCLIENT){  SetCursor(NULL);  return TRUE; } return FALSE;
	case WM_CLOSE: PostQuitMessage(0); return 0;
	case WM_SIZE: glViewport(0, 0, LOWORD(lParam), HIWORD(lParam)); return 0;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
		pMMI->ptMinTrackSize.x = 300;
		pMMI->ptMinTrackSize.y = 300;
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		int m_x, m_y, c_x, c_y;
		m_x = GET_X_LPARAM(lParam);
		m_y = GET_Y_LPARAM(lParam);
		c_x = rc.right;
		c_y = rc.bottom;

		bat_x = (((float)m_x / (float)c_x) * 2.0f) - 1.0f;
		bat_y = 1.0f - (((float)m_y / (float)c_y) * 2.0f);
		
		//sprintf_s(txt, sizeof(txt), "%d %d", m_x, m_y);		SetWindowTextA(hwnd, txt);

		bat_x = min(0.75f, max(-1.f, bat_x));
		bat_y = min(-0.8f, max(-0.9f, bat_y));
	}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE: PostQuitMessage(0); break;	
		case 'F':
		{
			if (!fullscreen) { SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS); ShowWindow(hwnd, SW_SHOWMAXIMIZED); fullscreen = true; break; }
			SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS); ShowWindow(hwnd, SW_SHOW); fullscreen = false; break; // should save rect before fullscreen
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

	static double ball_x_direction = ball_speed, ball_y_direction = -ball_speed, bounce_angle;
		
	if (ball_y-0.04f < bat_y) // ball bottom bounce off top bat
	{
		if (ball_x+0.04f > bat_x)
		{
			if (ball_x < bat_x + 0.25f)
			{
				bounce_angle = (ball_x - bat_x) - 0.125f; // from -0.125 to 0.125 
				ball_y_direction = -ball_y_direction;
				ball_x_direction = bounce_angle * 8.0 * ball_speed;
			}
		}
	}

	if (ball_x > 0.96f || ball_x < -0.96f) ball_x_direction = -ball_x_direction; // walls 
	if(ball_y>0.96f) ball_y_direction = -ball_y_direction; // top

	// bricks
	int n, m;
	float b_x, b_y;

	for (n = 0; n < 10; n++)
	for (m = 0; m < 5; m++)
	{
		b_x = -0.8f + (n / 6.0f); // top left of brick
		b_y = 0.8f - (m * 0.1f);

		if(ball_y_direction>0 && ball_y>b_y-0.09f && ball_y<b_y) // up hit bottom
			if (ball_x+0.04f > b_x && ball_x < b_x + 0.15f)
			{
				if (bricks[n][m] != 0)ball_y_direction = -ball_y_direction;
				bricks[n][m]=max(0,bricks[n][m]-1); 	
				continue;
			}

		if (ball_y_direction < 0 && ball_y - 0.04f < b_y && ball_y - 0.04f > b_y-0.09f) // down hit top
			if (ball_x+0.04f > b_x && ball_x < b_x + 0.15f)
			{
				if (bricks[n][m] != 0)ball_y_direction = -ball_y_direction;
				bricks[n][m] = max(0, bricks[n][m] - 1);
				continue;
			}

		if (ball_x_direction > 0 && ball_x + 0.04f > b_x && ball_x < b_x + 0.15f) // right hit left
			if (ball_y-0.04f < b_y && ball_y-0.04f > b_y - 0.09f)
			{
				if (bricks[n][m] != 0)ball_x_direction = -ball_x_direction;
				bricks[n][m] = max(0, bricks[n][m] - 1);
				continue;
			}

		if (ball_x_direction < 0 && ball_x < b_x + 0.15f && ball_x + 0.04f > b_x ) // left hit right
			if (ball_y - 0.04f < b_y && ball_y - 0.04f > b_y - 0.09f)
			{
				if (bricks[n][m] != 0)ball_x_direction = -ball_x_direction;
				bricks[n][m] = max(0, bricks[n][m] - 1);
			}
	}

	ball_x += ball_x_direction * t_diff;
	ball_y += ball_y_direction * t_diff;

	if (ball_y < -1.f) { lifes=max(0,lifes-1); ball_x = 0; ball_y = 0; ball_x_direction = ball_speed, ball_y_direction = -ball_speed; }

	// graphics 

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);

	glColor3f(0.1f, 0.5f, 1.f);
	glVertex2f(bat_x, bat_y); // user bat
	glVertex2f(bat_x + 0.25f, bat_y);
	glVertex2f(bat_x + 0.25f, bat_y - 0.06f);
	glVertex2f(bat_x, bat_y - 0.06f);

	glColor3f(0.3f, 0.5f, 1.f);
	glVertex2f(ball_x, ball_y); // ball
	glVertex2f(ball_x + 0.04f, ball_y);
	glVertex2f(ball_x + 0.04f, ball_y - 0.04f);
	glVertex2f(ball_x, ball_y - 0.04f);
	
	for(n=0;n<10;n++)
	for(m=0;m<5;m++)
	{
		switch (bricks[n][m])
		{		
		case 1: glColor3f(1.f, 0, 0); break;
		case 2: glColor3f(0, 1.f, 0); break;
		case 3: glColor3f(0,0,1.f); break;
		case 4: glColor3f(1.f,1.f,0); break;
		case 5: glColor3f(1.f, 1.f, 1.f); break;
		}
		
		b_x = -0.8f + (n / 6.0f);
		b_y = 0.8f - (m * 0.1f);

		if (bricks[n][m] != 0)
		{
			glVertex2f(b_x, b_y);
			glVertex2f(b_x + 0.15f, b_y);
			glVertex2f(b_x + 0.15f, b_y - 0.08f);
			glVertex2f(b_x, b_y - 0.08f);
		}
	}

	glEnd();

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2f(-0.25f, 0.85f);

	//sprintf_s(txt, sizeof(txt), "Qt=%ld %ld %f", qt_diff.QuadPart,qt_freq.QuadPart,t_diff);
	sprintf_s(txt, sizeof(txt), "%d", lifes);
	glPrintf(txt);
	glFlush();
	SwapBuffers(hdc);
}