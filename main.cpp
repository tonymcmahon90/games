#include <Windows.h>
#include <d2d1.h>
#pragma comment(lib,"D2d1.lib")
#include <d2d1_1.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool InitD2D(HWND hwnd);
void EndD2D();
void Paint();
void Resize(HWND hwnd);

ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRender = NULL;
ID2D1SolidColorBrush* pBrush = NULL;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WindowProc,0,0,hInst,NULL,NULL,NULL,NULL,L"Window" };
	RegisterClass(&wc);

	HWND hwnd = CreateWindow(L"Window", L"Direct2D sample", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);

	if (!InitD2D(hwnd)) return 0; // if fails still release() 
	
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
	if (FAILED(hr)) return false;
	
	RECT rc;
	GetClientRect(hwnd, &rc);
	
	hr = pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)), &pRender);
	if (FAILED(hr)) return false;

	hr = pRender->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Azure), &pBrush);
	if (FAILED(hr)) return false;

	return true;
}

void EndD2D()
{
	if (&pBrush) pBrush->Release();
	if (&pRender) pRender->Release();
	if (&pFactory) pFactory->Release(); 	
}

void Paint()
{
	pRender->BeginDraw();
	pRender->Clear(D2D1::ColorF(D2D1::ColorF::Blue));

	D2D1_ELLIPSE ellipse;

	for (int i = 0; i < 10; i++) 
	{		
		pBrush->SetColor(D2D1::ColorF((rand() % 100) / 100.f, (rand() % 100) / 100.f, (rand() % 100) / 100.f));
		ellipse = D2D1::Ellipse(D2D1::Point2F(rand() % 1000, rand() % 1000), rand() % 500, rand() % 500);
		pRender->DrawEllipse(ellipse, pBrush,rand()%20);

		pRender->DrawRectangle(D2D1::RectF(rand() % 1000, rand() % 1000, rand() % 500, rand() % 500), pBrush);		
	}
	pRender->EndDraw();
}

void Resize(HWND hwnd)
{
	if (pRender == NULL) return;

	RECT rc;
	GetClientRect(hwnd, &rc);
	
	D2D1_SIZE_U pixels = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	HRESULT hr=pRender->Resize(&pixels);

	if (FAILED(hr)) return;
}