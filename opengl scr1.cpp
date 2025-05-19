#include <windows.h>
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib") // needed for screensaver dialog
#include <scrnsave.h>
#pragma comment(lib,"scrnsavw.lib")
#include <stdio.h>
#include <gl/gl.h>
#include <gl/glu.h>
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glu32.lib")
#include <mmsystem.h>
#include <math.h>

#pragma warning(disable : 4996) 

#define IDS_DESCRIPTION		"ScreensaverGL" // name of screensaver

BOOL change, alpha = FALSE;

int* parts, * colours;
int Timer, x = 0, nParts = 500, size = 10, col[] = { 255,255,255,255 };

float v = 0, speed = 0.01f;
float xpos = 0, ypos = 0, zpos = 0, xlook = 1, ylook = 0, zlook = 0;
float ang = 160.0f, asp = 1.0f, zn = 1.0f, zf = 4000.0f;

HDC hDC;
HGLRC hRC;
RECT client;

SYSTEMTIME timea, timeb;

void render(HWND);
void randbits(void);
void EnableOpenGL(HWND);
void DisableOpenGL(HWND);
void openGL(void);
void GetSettings(void);

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CREATE:
		GetSettings(); // load setting file		
		parts = new int[nParts * 6];
		colours = new int[nParts * 4];
		EnableOpenGL(hwnd);

		glViewport(0, 0, client.right, client.bottom);
		glClearColor(0, 0, 0, 0);
		glEnable(GL_DEPTH_TEST);
		if (alpha) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
		}

		randbits();
		Timer = SetTimer(hwnd, 1, 10, NULL); // max 100fps
		break;
	case WM_TIMER:
		render(hwnd);
		break;
	case WM_DESTROY:
		KillTimer(hwnd, Timer);
		DisableOpenGL(hwnd);
		delete parts, colours;
		break;
	default:
		return DefScreenSaverProc(hwnd, message, wParam, lParam);
		render(hwnd);
	}
	return 0;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return 1;
}

void EnableOpenGL(HWND hwnd)
{
	hDC = GetDC(hwnd);
	GetClientRect(hwnd, &client);// full multi-mon 
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 16;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iFormat = ChoosePixelFormat(hDC, &pfd);
	SetPixelFormat(hDC, iFormat, &pfd);
	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
}

void DisableOpenGL(HWND hwnd)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hwnd, hDC);
}

void render(HWND hwnd)
{
	openGL();
	SwapBuffers(hDC);

	SYSTEMTIME t;
	GetSystemTime(&t);
	if (t.wSecond == 30) {
		if (change == TRUE) randbits();  // refresh once per minute 
	}
	if (t.wSecond == 29) change = TRUE;
}

void openGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluPerspective(ang, asp, zn, zf);
	gluLookAt(xpos, ypos, zpos, xlook, ylook, zlook, 0.0, 1.0, 0.0);

	xpos += (float)(sin(v));
	ypos -= (float)(cos(v));
	zpos += (float)(sin(v));
	xlook = xpos - (float)sin(v);
	ylook = ypos + (float)sin(v);
	zlook = zpos - (float)cos(v);

	v += speed; // move speed

	int c = 0, p = 0;

	glBegin(GL_QUAD_STRIP);
	for (int i = 0; i < nParts; i++) {
		glColor4ub(colours[c], colours[c + 1], colours[c + 2], colours[c + 3]);
		glVertex3i(parts[p], parts[p + 1], parts[p + 2]);
		glVertex3i(parts[p + 3], parts[p + 4], parts[p + 5]);
		p += 6; c += 4;
	}
	glEnd();
}

void randbits()
{
	SYSTEMTIME t;
	GetSystemTime(&t);
	srand(t.wMilliseconds);

	parts[0] = parts[1] = parts[2] = 0;
	for (x = 3; x < nParts * 6; x++) {
		parts[x] = parts[x - 3] + size - (rand() % (size * 2)); // makes new polygon near old one
		//		-1 > +1 = - size + ( 2 * size )
	}
	for (x = 0; x < nParts * 4; x += 4)
	{
		for (int y = 0; y < 4; y++)
		{
			if (col[y] != 0) colours[x + y] = rand() % col[y]; // r,g,b 0 crashes!
			else colours[x + y] = 0;
		}
	}
	change = FALSE;
}

void GetSettings()
{
	FILE* file = fopen("opengl.dat", "r");
	if (file == NULL) return;
	fscanf(file, "%d %f %d\r\n %d %d %d %d %d\r\n%f", &nParts, &ang, &size,
		&col[0], &col[1], &col[2], &col[3], &alpha,
		&speed);
	fclose(file);
}
