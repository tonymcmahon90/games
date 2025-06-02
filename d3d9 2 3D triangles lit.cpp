// shows two lit 3D triangles, press F11 to toggle fullscreen and ESC to quit
#include <Windows.h> 
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")
#include <DirectXMath.h>

using namespace DirectX;

#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR ) // https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dfvf 

struct CUSTOMVERTEX { FLOAT x, y, z; D3DVECTOR normal; DWORD diff; DWORD spec; };

CUSTOMVERTEX Verticies[] =
{
	{0.5f,1.0f,0.0f, 0.0f,0.0f,-1.0f,D3DCOLOR_XRGB(0,255,255),D3DCOLOR_XRGB(0,255,255) },    // triangle 0
	{1.0f,0.0f,0.0f, 0.0f,0.0f,-1.0f,D3DCOLOR_XRGB(255, 255, 0),D3DCOLOR_XRGB(0,255,255) },
	{0.0f,0.0f,0.0f, 0.0f,0.0f,-1.0f,D3DCOLOR_XRGB(255, 0, 255),D3DCOLOR_XRGB(0,255,255) },

	{-0.5f,1.0f,0.0f, 0.0f,0.0f,-1.0f,D3DCOLOR_XRGB(0,0,255),D3DCOLOR_XRGB(0,255,255) },	// triangle 1
	{0.0f,0.0f,0.0f, 0.0f,0.0f,-1.0f,D3DCOLOR_XRGB(0, 255, 0),D3DCOLOR_XRGB(0,255,255) },
	{-1.0f,0.0f,0.0f, 0.0f,0.0f,-1.0f,D3DCOLOR_XRGB(255,0,0),D3DCOLOR_XRGB(0,255,255) },
};

LPDIRECT3D9 d3d = NULL;
LPDIRECT3DDEVICE9 d3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;
D3DPRESENT_PARAMETERS d3dpp;
D3DDISPLAYMODE mode3d; // details for fullscreen 
RECT rc; // client size 

HWND hwnd = NULL;
int width, height;
int fps = 0;
WCHAR txt[255],classname[]=L"D3D9 Window";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitD3D();
void EndD3D();
void RenderD3D();
void ResizeD3D();
void ToggleFullscreenD3D();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL, IDC_ARROW),(HBRUSH)GetStockObject(WHITE_BRUSH),NULL,classname };
	RegisterClass(&wc);
		
	hwnd = CreateWindow(classname, classname, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd,show);
	
	InitD3D();
		
	SYSTEMTIME st,nt; // system time, now time 
	GetSystemTime(&st);
		
	MSG msg = {};

	while(true)
	{ 
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // if there is a windows message process it
		{
			if( msg.message == WM_QUIT) break; // end the while loop, end the program 
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// game code 
			RenderD3D();
			fps++;
			GetSystemTime(&nt);
			if (nt.wSecond != st.wSecond)
			{
				wsprintf(txt, L"%s %d fps width=%d height=%d",classname,fps,width,height);
				SetWindowText(hwnd, txt);
				st.wSecond = nt.wSecond;
				fps = 0;
			}
		}
	}

	EndD3D();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
		case WM_KEYUP:
		{
			if (LOWORD(wparam) == VK_F11) { ToggleFullscreenD3D(); return 0; }
			if (LOWORD(wparam) == VK_ESCAPE) { PostQuitMessage(0); return 0; }
			return 0;
		}
		case WM_SIZE:
		{
			ResizeD3D(); return 0;
		}
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* pMMI = (MINMAXINFO*)lparam;
			pMMI->ptMinTrackSize.x = 300;
			pMMI->ptMinTrackSize.y = 300;
			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0); return 0;
		}		
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

bool InitD3D()
{	
	GetClientRect(hwnd, &rc);	
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice))) return false;
		
	d3dDevice->GetDisplayMode(0, &mode3d);

	d3dDevice->CreateVertexBuffer(6 * sizeof(CUSTOMVERTEX), 0, CUSTOMFVF, D3DPOOL_MANAGED, &v_buffer, NULL); // 2 triangles 

	VOID* pVoid;
	v_buffer->Lock(0, 0, (void**)&pVoid, 0);
	memcpy(pVoid, Verticies, sizeof(Verticies));
	v_buffer->Unlock();
	
	return true;
}

void RenderD3D()
{
	if (d3dDevice == NULL) return;

	D3DMATERIAL9 mat;	
	mat.Ambient = { 1.0f, 0.0f, 1.0f, 1.0f };
	mat.Diffuse = { 0.5f, 0.5f, 1.0f, 1.0f };
	mat.Power = 1.0f;
	d3dDevice->SetMaterial(&mat);

	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	light.Direction = { -1.0f,-0.5f,-1.0f };
	d3dDevice->SetLight(0, &light);
	d3dDevice->LightEnable(0, TRUE);

	d3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE); // using custom 
	d3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(50, 50, 50));

	d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); // draw backface 

	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 100), 1.0f, 0);
	d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	d3dDevice->BeginScene();

	d3dDevice->SetFVF(CUSTOMFVF);

	static float angle = 0.0f;
	struct constbuffer { XMMATRIX transform; };
	const constbuffer world = { { XMMatrixRotationY(angle) } };
	angle += (1.0f/57.29f);

	d3dDevice->SetTransform(D3DTS_WORLD,(D3DMATRIX*)& world);

	XMVECTOR eye = XMVectorSet(0.0f, 0.5f, 3.0f, 1.0f), lookat = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	const constbuffer view = { { XMMatrixLookAtLH(eye,lookat,up)} };
		
	d3dDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);

	const constbuffer proj = { { XMMatrixPerspectiveFovLH(45.f / 57.29f,float(width) / float(height),1.0f,30.f)} };

	d3dDevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);

	d3dDevice->SetStreamSource(0, v_buffer, 0, sizeof(CUSTOMVERTEX));
	d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

	const constbuffer world2 = { { XMMatrixMultiply(XMMatrixRotationY(angle+0.5f),XMMatrixTranslation(1.0f,0.1f,0.0f)) }};
	d3dDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&world2);
	d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
	d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

	d3dDevice->EndScene();
	d3dDevice->Present(NULL, NULL, NULL, NULL);
}

void ResizeD3D()
{
	if (d3dDevice == NULL) return;
	
	ZeroMemory(&d3dpp, sizeof(d3dpp));		
	if(GetWindowLong(hwnd, GWL_STYLE) & WS_OVERLAPPEDWINDOW)
	{
		d3dpp.Windowed = TRUE; // uses client size if width or height is 0

		GetClientRect(hwnd, &rc);
		width = rc.right - rc.left;
		height = rc.bottom - rc.top; // shown in setwindowtext	
	}
	else
	{
		d3dpp.Windowed = FALSE; // fullscreen 
		d3dpp.FullScreen_RefreshRateInHz = mode3d.RefreshRate; // use orignial info 
		d3dpp.BackBufferWidth = mode3d.Width;
		d3dpp.BackBufferHeight = mode3d.Height;
		d3dpp.BackBufferFormat = mode3d.Format;

		//wsprintf(txt, L"hz=%d width=%d height=%d format=%d", mode3d.RefreshRate, mode3d.Width, mode3d.Height, mode3d.Format);
		//MessageBox(hwnd, txt, L"", NULL);
	}
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;	
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	if (d3dDevice->Reset(&d3dpp) == D3DERR_DEVICELOST) MessageBox(hwnd, L"Lost", L"Error", NULL);
}

void ToggleFullscreenD3D()
{
	LONG style=GetWindowLong(hwnd, GWL_STYLE);

	static RECT prev;

	if(style & WS_OVERLAPPEDWINDOW)
	{
		GetWindowRect(hwnd, &prev); // save when we got back to windowed 
		//wsprintf(txt, L"%d %d %d %d", prev.left, prev.top, prev.right-prev.left, prev.bottom-prev.top);
		//MessageBox(hwnd, txt, L"", NULL);
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
		SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);
		ShowWindow(hwnd, SW_MAXIMIZE);		
	}		
	else
	{		
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowLong(hwnd, GWL_EXSTYLE, 0);		
		ShowWindow(hwnd, SW_NORMAL);
		MoveWindow(hwnd, prev.left, prev.top, prev.right - prev.left, prev.bottom - prev.top, FALSE);
	}			

	ResizeD3D();
}

void EndD3D()
{
	if (v_buffer) v_buffer->Release();
	if (d3dDevice) d3dDevice->Release();
	if (d3d) d3d->Release();
}