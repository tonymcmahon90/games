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
float bat_x = -1.f, bat_y = 0, c_bat_x = 1.f, c_bat_y = 0, ball_x = 0, ball_y = 0, ball_speed = 0.6f,c_react_x = 0.2f;
double t_diff = 0, skill=1.0;
int score = 0, c_score = 0;
bool hide_cursor = false, color = false, fixed_x = false;
char txt[1000];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	WNDCLASS wc = { CS_OWNDC | CS_VREDRAW | CS_HREDRAW,WndProc,0,0,hInst,NULL,LoadCursor(NULL,IDC_ARROW),NULL,NULL,L"Ping Pong" }; 
	RegisterClass(&wc);
	
	hwnd = CreateWindow(L"Ping Pong", L"Ping Pong", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
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
	case WM_SETCURSOR: if (LOWORD(lParam) == HTCLIENT) { if (hide_cursor) SetCursor(NULL); else SetCursor(LoadCursor(NULL, IDC_ARROW)); return TRUE; } return FALSE;
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
		int m_x, m_y,c_x,c_y;
		m_x = GET_X_LPARAM(lParam);
		m_y = GET_Y_LPARAM(lParam);
		c_x = rc.right;
		c_y = rc.bottom;

		bat_x = (((float)m_x / (float)c_x)*2.0f)-1.0f;
		bat_y= 1.0f-(((float)m_y / (float)c_y) * 2.0f);
		if (fixed_x)bat_x = -1.0f;

		//sprintf_s(txt, sizeof(txt), "%d %d", m_x, m_y);		SetWindowTextA(hwnd, txt);

		bat_x = min(-0.1f,max(-1.f, bat_x));
		bat_y = min(0.98f,max(-0.73f, bat_y));
	}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE: PostQuitMessage(0); break;
		case 'H': hide_cursor = !hide_cursor; break;
		case 'C': color = !color; break;
		case 'X': fixed_x = !fixed_x; break;
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

	static LARGE_INTEGER qt, qt_prev,qt_diff,qt_freq; 
	QueryPerformanceFrequency(&qt_freq);
	if(qt_prev.QuadPart==0) QueryPerformanceCounter(&qt_prev);
	QueryPerformanceCounter(&qt);
	qt_diff.QuadPart = qt.QuadPart - qt_prev.QuadPart;
	double tmp = (double)qt_diff.QuadPart / (double)qt_freq.QuadPart;
	if (tmp < (1.0 / 60.0)) return; // No more than 60fps
	
	t_diff = tmp;
	qt_prev.QuadPart = qt.QuadPart;

	// game logic

	static double ball_x_direction=ball_speed, ball_y_direction=ball_speed,bounce_angle;

	if (ball_x + (ball_x_direction * t_diff) > c_bat_x) // bounce off c_bat
	{
		if (ball_y + (ball_y_direction * t_diff) < c_bat_y)
		{
			if (ball_y + (ball_y_direction * t_diff) > c_bat_y - 0.25f)
			{
				bounce_angle = (ball_y - c_bat_y)-0.125f;
				ball_x_direction = -ball_x_direction+bounce_angle;
			}
		}
	}

	if (ball_x + (ball_x_direction * t_diff) < bat_x) // bounce off bat
	{
		if (ball_y + (ball_y_direction * t_diff) < bat_y)
		{
			if (ball_y + (ball_y_direction * t_diff) > bat_y - 0.25f)
			{
				bounce_angle = (ball_y - bat_y)-0.125f;
				ball_x_direction = -ball_x_direction + bounce_angle;
			}
		}
	}

	if (ball_y > 0.98f || ball_y<-0.98f) ball_y_direction = -ball_y_direction;

	ball_x += ball_x_direction * t_diff;
	ball_y += ball_y_direction * t_diff;

	if (ball_x > 1.f) { score++; ball_x = 0; ball_y = 0; ball_x_direction = ball_speed, ball_y_direction = ball_speed; }
	if (ball_x <-1.f) { c_score++; ball_x = 0; ball_y = 0; ball_x_direction = ball_speed, ball_y_direction = ball_speed; }
	if (score > 10 || c_score > 10) { score = c_score = 0; }

	// move computer bat

	if (ball_x > c_react_x) // only react on side 
	{
		if (c_bat_y - 0.125f > ball_y) c_bat_y -= skill * t_diff;
		if (c_bat_y - 0.125f < ball_y) c_bat_y += skill * t_diff;
	}
	// graphics 
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_QUADS);
	if(color)glColor3f(0.f, 1.f, 0.f); else glColor3f(1.f, 1.f, 1.f);

	glVertex2f(-0.01f,-1.f); // centre
	glVertex2f(0.01f, -1.f);
	glVertex2f(0.01f, 1.f);
	glVertex2f(-0.01f, 1.f);

	glVertex2f(-1.f, -1.f); // bottom
	glVertex2f(1.f, -1.f);
	glVertex2f(1.f, -0.98f);
	glVertex2f(-1.f, -0.98f);

	glVertex2f(-1.f, 1.f); // top
	glVertex2f(1.f, 1.f);
	glVertex2f(1.f,0.98f);
	glVertex2f(-1.f,0.98f);

	if (color)glColor3f(1.f, 0.f, 0.f);
	glVertex2f(bat_x, bat_y); // user bat
	glVertex2f(bat_x+0.02f, bat_y);
	glVertex2f(bat_x+0.02f, bat_y-0.25f);
	glVertex2f(bat_x, bat_y-0.25f);

	if (color)glColor3f(0.f, 0.f, 1.f);
	glVertex2f(c_bat_x, c_bat_y); // computer bat
	glVertex2f(c_bat_x - 0.02f, c_bat_y);
	glVertex2f(c_bat_x - 0.02f, c_bat_y - 0.25f);
	glVertex2f(c_bat_x, c_bat_y - 0.25f);

	if (color)glColor3f(1.f, 1.f, 1.f);
	glVertex2f(ball_x, ball_y); // ball
	if (color)glColor3f(1.f, 0.f, 1.f);
	glVertex2f(ball_x + 0.02f, ball_y);
	if (color)glColor3f(0.f, 0.f, 1.f);
	glVertex2f(ball_x + 0.02f, ball_y - 0.02f);
	if (color)glColor3f(0.f, 1.f, 0.f);
	glVertex2f(ball_x, ball_y - 0.02f);

	glEnd();

	glColor3f(1.f,1.f,1.f);
	glRasterPos2f(-0.25f, 0.85f);	

	
	//sprintf_s(txt, sizeof(txt), "Qt=%ld %ld %f", qt_diff.QuadPart,qt_freq.QuadPart,t_diff);
	sprintf_s(txt, sizeof(txt), "%d %d",score,c_score);
	glPrintf(txt);
	glFlush();
	SwapBuffers(hdc);
}