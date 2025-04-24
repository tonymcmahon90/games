#include <Windows.h> // make sure linker/system/subsystem is Windows 
#include <d2d1.h>
#pragma comment(lib,"D2d1.lib")
#include <d2d1_1.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool InitD2D(HWND hwnd);
void EndD2D();
void Paint();
bool Resize(HWND hwnd);

ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRender = NULL;
ID2D1SolidColorBrush* pBrush = NULL;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WindowProc,0,0,hInst,NULL,NULL,NULL,NULL,L"Window" }; 
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(L"Window", L"Direct2D sample 1", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);
	InitD2D(hwnd);
	
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	EndD2D();
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT: Paint(); break;
	case WM_CLOSE: DestroyWindow(hwnd); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_SIZE: Resize(hwnd); break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool InitD2D(HWND hwnd)
{
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	if (FAILED(hr)) { MessageBox(hwnd, L"D2D Factory", NULL, MB_ICONERROR);  PostQuitMessage(0); return false; }
	
	RECT rc;
	GetClientRect(hwnd, &rc);
	
	hr = pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)), &pRender);
	if (FAILED(hr)) { MessageBox(hwnd, L"D2D Render", NULL, MB_ICONERROR); PostQuitMessage(0); return false; }

	hr = pRender->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Azure), &pBrush);
	if (FAILED(hr)) { MessageBox(hwnd, L"D2D Brush", NULL, MB_ICONERROR); PostQuitMessage(0); return false; }

	return true;
}

void EndD2D()
{
	if (&pBrush) { pBrush->Release(); pBrush = NULL; }
	if (&pRender) {	pRender->Release(); pRender = NULL;	}
	if (&pFactory) { pFactory->Release(); pFactory = NULL; }
}

void Paint()
{
	if (pRender == NULL || pBrush == NULL) return;

	pRender->BeginDraw();
	pRender->Clear(D2D1::ColorF(D2D1::ColorF::Blue));

	D2D1_ELLIPSE ellipse;

	for (int i = 0; i < 50; i++) 
	{		
		pBrush->SetColor(D2D1::ColorF((rand() % 200) / 200.f, (rand() % 200) / 200.f, (rand() % 200) / 200.f));
		ellipse = D2D1::Ellipse(D2D1::Point2F((float)(rand() % 1000),(float)(rand() % 1000) ), (float)(rand() % 1000), (float)(rand() % 1000) );
		pRender->DrawEllipse(ellipse, pBrush,(float)(rand()%20) );

		pRender->DrawRectangle(D2D1::RectF((float)(rand() % 1000), (float)(rand() % 1000), (float)(rand() % 1000), (float)(rand() % 1000) ), pBrush, (float)(rand() % 20) );
	}
	pRender->EndDraw();
}

bool Resize(HWND hwnd)
{
	if (pRender == NULL) return false;

	RECT rc;
	GetClientRect(hwnd, &rc);
	
	D2D1_SIZE_U pixels = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	HRESULT hr=pRender->Resize(&pixels);

	if (FAILED(hr)) return false;

	return true;
}