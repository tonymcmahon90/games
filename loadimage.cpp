#include <Windows.h>
#include <Windowsx.h> // mouse pos
#include <stdio.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hwnd;
int x, y;
char txt[1000];

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR cmd, int show)
{
	WNDCLASS wc = { 0,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL,IDC_ARROW),(HBRUSH)GetStockObject(BLACK_BRUSH),NULL,L"Simple Window"};
	RegisterClass(&wc);

	hwnd = CreateWindow(L"Simple Window", L"Simple Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);
	MSG msg = {};

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

void ShowImage()
{	
	HBITMAP hBitmap=(HBITMAP)LoadImage(NULL, L"tree.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	HDC hdc = GetDC(hwnd),memdc=CreateCompatibleDC(hdc);
	HGDIOBJ hBMP = SelectObject(memdc, hBitmap);

	BITMAP bm;
	GetObject(hBitmap, sizeof(BITMAP), &bm);
	BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, memdc, 0, 0, SRCCOPY);
	SetStretchBltMode(hdc, HALFTONE); // fix colour when stretched 
	SetBrushOrgEx(hdc, 0, 0, NULL); // needed ? seems not 
	StretchBlt(hdc, bm.bmWidth+10, 0, bm.bmWidth / 2, bm.bmHeight / 2, memdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
	ReleaseDC(hwnd, memdc);
		
	sprintf_s(txt, 100, "Width %d Height %d Bits %d", bm.bmWidth, bm.bmHeight,bm.bmBitsPixel);
	SetTextColor(hdc, 0x00ffffff);
	SetBkMode(hdc, TRANSPARENT);
	TextOutA(hdc, 10, 10, txt, strlen(txt));

	DeleteDC(memdc);
	ReleaseDC(hwnd,hdc);
}

void PixelColor()
{
	HDC hdc = GetDC(hwnd);
	COLORREF c=GetPixel(hdc, x, y);
	sprintf_s(txt, 100, "X %d Y %d %X               ", x, y,c);
	SetTextColor(hdc, 0x00ffffff);
	SetBkColor(hdc, 0x00000000);
	SetBkMode(hdc, OPAQUE);
	TextOutA(hdc, 10, 30, txt, strlen(txt));

	ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE:
		x = GET_X_LPARAM(lParam); 
		y = GET_Y_LPARAM(lParam);
		PixelColor();
		return 0;
	case WM_KEYDOWN:
		if (wParam == 'L') ShowImage();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}