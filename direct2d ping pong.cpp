#include <Windows.h> // make sure linker/system/subsystem is Windows 
#include <d2d1.h>
#pragma comment(lib,"D2d1.lib")
#include <d2d1_1.h>
#include <strsafe.h>
#include <windowsx.h> // GET_X_LPARAM
#include <math.h>
#include "resource.h"

#define FPS 10

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool InitD2D(HWND hwnd);
void EndD2D();
void Paint();
bool Resize(HWND hwnd);

ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRender = NULL;
ID2D1SolidColorBrush* pBrush = NULL;
ID2D1LinearGradientBrush* pLinBrush = NULL; // https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-brushes-overview 
ID2D1RadialGradientBrush* pGradBrush = NULL;

LONG width, height,initwidth,initheight;
float user_bat_x = 100, user_bat_y = 300,mouse_x=-1,mouse_y=-1,ball_x=600,ball_y=250;
float ball_play = -1,ball_x_direction=-1,ball_y_direction=-1;
float ai_y = 300;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WindowProc,0,0,hInst,(HICON)LoadImage(hInst,MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,LR_DEFAULTSIZE,LR_DEFAULTSIZE,LR_SHARED)
		//LoadIcon(NULL, IDI_APPLICATION)
		,LoadCursor(NULL, IDC_ARROW),NULL,NULL,L"Window" };
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(L"Window", L"Direct2D Ping Pong", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);

	RECT rc;
	GetClientRect(hwnd, &rc);
	initwidth = rc.right - rc.left;
	initheight = rc.bottom - rc.top;
	InitD2D(hwnd);
			
	LARGE_INTEGER qtime,qfreq,qtemp;
	QueryPerformanceCounter(&qtime);
	QueryPerformanceFrequency(&qfreq);
	qfreq.QuadPart = qfreq.QuadPart / FPS;

	// WCHAR txt[50]; StringCbPrintfW(txt,sizeof(WCHAR)*50, L" F=%d", qfreq.QuadPart); MessageBox(NULL,txt, NULL, 0);
	
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{			
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);			
		}
		else
		{
			QueryPerformanceCounter(&qtemp);

			if (qtemp.QuadPart < ( qtime.QuadPart + qfreq.QuadPart) ) continue;
				
			Paint();
			qtime.QuadPart = qtemp.QuadPart;
		}
	}

	// MessageBox(NULL, L"Done", NULL, NULL);
	EndD2D();
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	//case WM_PAINT: Paint(); break; // now using the timer 
	case WM_CLOSE: DestroyWindow(hwnd); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_SIZE: Resize(hwnd); break;
	case WM_MOUSEMOVE: 
		if (MK_RBUTTON && wParam) { user_bat_x = 100; user_bat_y = 300; mouse_x = GET_X_LPARAM(lParam); mouse_y = GET_Y_LPARAM(lParam); return 0; } // reset 

		if (mouse_x == -1 || MK_LBUTTON && wParam) // left click to begin or reset bat movement 
		{
			if (MK_LBUTTON && wParam) { mouse_x = GET_X_LPARAM(lParam); mouse_y = GET_Y_LPARAM(lParam); } // start when use clicks left mouse
			return 0;
		}	
		
		int x_diff= GET_X_LPARAM(lParam) - mouse_x;
		int y_diff= GET_Y_LPARAM(lParam) - mouse_y;
		user_bat_x += x_diff;
		user_bat_y += y_diff;
		mouse_x = GET_X_LPARAM(lParam);
		mouse_y = GET_Y_LPARAM(lParam);

		if (user_bat_x < 50)user_bat_x = 50;
		if (user_bat_x > 250)user_bat_x = 250;

		if (user_bat_y < 100)user_bat_y = 100;
		if (user_bat_y > 500)user_bat_y = 500;
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool InitD2D(HWND hwnd)
{
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	if (FAILED(hr)) { MessageBox(hwnd, L"D2D Factory", NULL, MB_ICONERROR);  PostQuitMessage(0); return false; }

	RECT rc;
	GetClientRect(hwnd, &rc);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	hr = pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(width, height)), &pRender);
	if (FAILED(hr)) { MessageBox(hwnd, L"D2D Render", NULL, MB_ICONERROR); PostQuitMessage(0); return false; }

	hr = pRender->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Azure), &pBrush);
	if (FAILED(hr)) { MessageBox(hwnd, L"D2D Brush", NULL, MB_ICONERROR); PostQuitMessage(0); return false; }

	return true;
}

void EndD2D()
{
	if (pGradBrush) { pGradBrush->Release(); pGradBrush = NULL; }
	if (pLinBrush) { pLinBrush->Release(); pLinBrush = NULL; }
	if (pBrush) { pBrush->Release(); pBrush = NULL; }
	if (pRender) { pRender->Release(); pRender = NULL; }
	if (pFactory) { pFactory->Release(); pFactory = NULL; }
}

void Paint()
{
	if (pRender == NULL || pBrush == NULL) return;

	float xscale = width / initwidth,yscale=height/initheight;

	pRender->BeginDraw();
	pRender->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// table edges and middle
	pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
	pRender->DrawRectangle(D2D1::RectF(100,100,900,100), pBrush,10.f); // top bar
	pRender->DrawRectangle(D2D1::RectF(100,500,900,500), pBrush, 10.f); // bottom bar
	pRender->DrawRectangle(D2D1::RectF(500, 100, 500, 500), pBrush, 10.f); // middle

	// bats
	pRender->DrawRectangle(D2D1::RectF(user_bat_x, user_bat_y-50,user_bat_x, user_bat_y+50), pBrush, 10.f); // left user
	pRender->DrawRectangle(D2D1::RectF(900, ai_y-50, 900, ai_y+50), pBrush, 10.f); // right AI

	// ball
	pRender->DrawRectangle(D2D1::RectF(ball_x,ball_y,ball_x,ball_y), pBrush, 15.f); // ball

	if (ball_play == -1) { // start game 
		ball_x_direction = -10;
		ball_y_direction = -10;

		ball_x += ball_x_direction;
		ball_y += ball_y_direction;
		ball_play = 0;
	}

	if(ball_x-user_bat_x<20 && abs((int)ball_y-(int)user_bat_y)<50) ball_x_direction = 10;

	// bounce around table

	if (ball_x < 50) {
		MessageBox(NULL, L"Opponent wins a point", NULL, MB_OK); ball_x = 500;
	}
	if (ball_y < 110) ball_y_direction = 10;
	if (ball_x > 900) {
		MessageBox(NULL, L"You win a point", NULL, MB_OK); ball_x = 500; ai_y = 300;
	}
	if (ball_y > 490) ball_y_direction = -10;

	// bounce off bat 

	if (ball_x - user_bat_x < 20 && abs((int)ball_y - (int)user_bat_y) < 50) ball_x_direction = 10;

	if (900-ball_x < 20 && abs((int)ball_y - (int)ai_y) < 50) ball_x_direction = -10;

	ball_x += ball_x_direction;
	ball_y += ball_y_direction;

	// AI

	if (ball_x > 700)
	{
		if (ai_y < ball_y) ai_y += 10; else if (ai_y > ball_y) ai_y -= 10;
	}
	/*
	D2D1_ELLIPSE ellipse;

	for (int i = 0; i < 10; i++)
	{
		pBrush->SetColor(D2D1::ColorF((rand() % 200) / 200.f, (rand() % 200) / 200.f, (rand() % 200) / 200.f));
		ellipse = D2D1::Ellipse(D2D1::Point2F((float)(rand() % 1000), (float)(rand() % 1000)), (float)(rand() % 1000), (float)(rand() % 1000));
		pRender->DrawEllipse(ellipse, pBrush, (float)(rand() % 20));

		pRender->DrawRectangle(D2D1::RectF((float)(rand() % 1000), (float)(rand() % 1000), (float)(rand() % 1000), (float)(rand() % 1000)), pBrush, (float)(rand() % 20));
	}*/
	pRender->EndDraw();
}

bool Resize(HWND hwnd)
{
	if (pRender == NULL) return false;

	RECT rc;
	GetClientRect(hwnd, &rc);
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	D2D1_SIZE_U pixels = D2D1::SizeU(width, height);
	HRESULT hr = pRender->Resize(&pixels);

	if (FAILED(hr)) return false;

	return true;
}