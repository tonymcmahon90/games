// rename .exe to .scr and right click to Test 
#include <Windows.h>
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib") // needed for screensaver dialog
#include <scrnsave.h>
#pragma comment(lib,"scrnsavw.lib") // UNICODE 

#define IDS_DESCRIPTION	"Screensaver1" // name of screensaver

LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HDC hdc;
	static int width, height;
	static RECT rc;
	PAINTSTRUCT ps;
	int x, y;
	
	switch (msg)
	{
	case WM_CREATE:
		GetClientRect(hwnd, &rc);
		width = rc.right;
		height = rc.bottom;
		SetTimer(hwnd, 1, 500, NULL);
		return 0;
	case WM_DESTROY:
		KillTimer(hwnd, 1);
		return 0;
	case WM_TIMER: 
		InvalidateRect(hwnd, &rc, TRUE);
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);			
		SelectObject(hdc, GetStockObject(DC_BRUSH));
		for (int n = 0; n < 10; n++)
		{
			SetDCBrushColor(hdc, RGB(rand()%255, rand() % 255, rand() % 255));
			x = rand() % width;
			y = rand() % height;
			Rectangle(hdc,x,y,x+width/20,y+height/20);
		}
		EndPaint(hwnd, &ps);
		return 0;
	default:
		return DefScreenSaverProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hinst)
{
	return TRUE;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return FALSE;
}