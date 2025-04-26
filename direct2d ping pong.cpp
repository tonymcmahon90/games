#include <Windows.h> // make sure linker/system/subsystem is Windows 
#include <d2d1.h>
#pragma comment(lib,"D2d1.lib")
#include <d2d1_1.h>
#include <strsafe.h>
#include <windowsx.h> // GET_X_LPARAM
#include <math.h> // fabs

#define FPS 60
#define BALLSPEED 0.002

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

LONG width,height;
bool game_play = false;
float user_bat_x = 0.1, user_bat_y = 0.5,mouse_x=-1,mouse_y=-1,ball_x=0.5,ball_y=0.5,ball_x_direction,ball_y_direction,ai_y = 0.5,bat_size=0.035,ball_speed=BALLSPEED,ball_accel=1+(BALLSPEED/5);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WindowProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL, IDC_ARROW),NULL,NULL,L"Window" };
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(L"Window", L"Direct2D Ping Pong", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);
	InitD2D(hwnd);
			
	LARGE_INTEGER qtime,qfreq,qtemp;
	QueryPerformanceCounter(&qtime);
	QueryPerformanceFrequency(&qfreq);
	qfreq.QuadPart = qfreq.QuadPart / FPS;
		
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
	EndD2D();
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{	
	case WM_CLOSE: DestroyWindow(hwnd); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_SIZE: Resize(hwnd); break;
	case WM_MOUSEMOVE: 		
		if (MK_LBUTTON && wParam) // left click to begin 
		{
			mouse_x = GET_X_LPARAM(lParam); 
			mouse_y = GET_Y_LPARAM(lParam);  // start when use clicks left mouse
			return 0;
		}	
		
		int x_diff= GET_X_LPARAM(lParam) - mouse_x;
		int y_diff= GET_Y_LPARAM(lParam) - mouse_y;
		user_bat_x += (float)x_diff/width;
		user_bat_y += (float)y_diff/height;
		mouse_x = GET_X_LPARAM(lParam);
		mouse_y = GET_Y_LPARAM(lParam);

		if (user_bat_x < 0.1)user_bat_x = 0.1;
		if (user_bat_x > 0.45)user_bat_x = 0.45;
		if (user_bat_y < 0.1)user_bat_y = 0.1;
		if (user_bat_y > 0.9)user_bat_y = 0.9;
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

	pRender->BeginDraw();
	pRender->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// table edges and middle
	pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
	pRender->DrawRectangle(D2D1::RectF(0.1*width,0.1*height,0.9*width,0.1*height), pBrush,0.005*width); // top bar
	pRender->DrawRectangle(D2D1::RectF(0.1*width,0.9*height,0.9*width,0.9*height), pBrush,0.005*width); // bottom bar
	pRender->DrawRectangle(D2D1::RectF(0.5*width,0.1*height,0.5*width,0.9*height), pBrush,0.005*width); // middle

	// bats
	pRender->DrawRectangle(D2D1::RectF(user_bat_x*width, (user_bat_y-bat_size)*height,user_bat_x*width, (user_bat_y + bat_size) * height ),  pBrush, 0.005 * width); // left user
	pRender->DrawRectangle(D2D1::RectF(0.9*width, (ai_y - bat_size) * height, 0.9*width, (ai_y + bat_size) * height ), pBrush, 0.005 * width); // right AI

	// ball
	pRender->DrawRectangle(D2D1::RectF(ball_x*width,ball_y*height,ball_x*width,ball_y*height), pBrush, 0.0075 * width); // ball

	if (!game_play) { // start game 
		ball_speed = BALLSPEED;
		ball_x_direction = -ball_speed;
		ball_y_direction = -ball_speed;		
		game_play = true;
	}

	// bounce off user bat
	if (fabs(ball_x-user_bat_x)<0.01 && fabs(ball_y - user_bat_y)<bat_size) ball_x_direction=ball_speed;

	// ball lost
	if (ball_x < 0.09) { MessageBox(NULL, L"Opponent wins a point", L"", MB_OK); ball_x = 0.5; ai_y = 0.5; game_play = false; }
	if (ball_x > 0.91) { MessageBox(NULL, L"You win a point", L"", MB_OK); ball_x = 0.5; ai_y = 0.5; game_play = false; }

	// ball bounce off top or bottom
	if (ball_y-ball_speed-0.005 < 0.1) ball_y_direction = ball_speed;
	if (ball_y+ball_speed+0.005 > 0.9) ball_y_direction = -ball_speed;

	// bounce off AI bat 
	if (fabs(0.9-ball_x)<0.01 && fabs(ball_y - ai_y) < bat_size) ball_x_direction = -ball_speed;

	// AI respond once ball is on side
	if (ball_x > 0.5){ if (ai_y < ball_y) ai_y += ball_speed; else if (ai_y > ball_y) ai_y -= ball_speed; }	

	// move ball
	ball_x += ball_x_direction;
	ball_y += ball_y_direction;
	ball_speed *= ball_accel;
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