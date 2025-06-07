// shows two cubes, ESC to quit
#include <Windows.h> 
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")
#include <DirectXMath.h>

using namespace DirectX;

#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE  ) // https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dfvf 

struct CUSTOMVERTEX { FLOAT x, y, z; DWORD diff; };

CUSTOMVERTEX Verticies[] =  // 8 to form a cube 
{
	{-0.5f,0.5f,0.5f, D3DCOLOR_XRGB(255,0,0), },    
	{0.5f,0.5f,0.5f, D3DCOLOR_XRGB(255, 255, 0), },
	{0.5f,0.5f,-0.5f, D3DCOLOR_XRGB(255, 0, 255), },
	{-0.5f,0.5f,-0.5f, D3DCOLOR_XRGB(0,0,255), },
	{-0.5f,-0.5f,0.5f, D3DCOLOR_XRGB(0, 255, 255), },
	{0.5f,-0.5f,0.5f, D3DCOLOR_XRGB(0,0,0), },
	{0.5f,-0.5f,-0.5f, D3DCOLOR_XRGB(0, 255, 0), },
	{-0.5f,-0.5f,-0.5f, D3DCOLOR_XRGB(255,255,255), },
};

unsigned short Indicies[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7, 0,1,5, 0,5,4, 3,0,4, 4,7,3, 3,2,6, 3,6,7, 2,1,5, 2,5,6,  }; // top and bottom sides 12*3=36

LPDIRECT3D9 d3d = NULL;
LPDIRECT3DDEVICE9 d3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;
LPDIRECT3DINDEXBUFFER9 i_buffer = NULL;
D3DPRESENT_PARAMETERS d3dpp;

HWND hwnd = NULL;
int fps = 0;
WCHAR txt[255],classname[]=L"D3D9 Window";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitD3D();
void EndD3D();
void RenderD3D();
void ResizeD3D();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL, IDC_ARROW),(HBRUSH)GetStockObject(WHITE_BRUSH),NULL,classname };
	RegisterClass(&wc);
		
	hwnd = CreateWindow(classname, classname, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd,show);
	
	InitD3D();	
		
	MSG msg = {};

	while(true)
	{ 
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // if there is a windows message process it
		{			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) break; // end the while loop, end the program 
		}
		else
			RenderD3D();
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
			pMMI->ptMinTrackSize.x = 400;
			pMMI->ptMinTrackSize.y = 400;
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
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice))) return false;
		
	d3dDevice->CreateVertexBuffer(8 * sizeof(CUSTOMVERTEX), 0, CUSTOMFVF, D3DPOOL_MANAGED, &v_buffer, NULL); 

	VOID* pVoid;
	v_buffer->Lock(0, 0, (void**)&pVoid, 0);
	memcpy(pVoid, Verticies, sizeof(Verticies));
	v_buffer->Unlock();

	d3dDevice->CreateIndexBuffer(36 * sizeof(unsigned short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &i_buffer, NULL);

	i_buffer->Lock(0, 0, (void**)&pVoid, 0);
	memcpy(pVoid, Indicies, sizeof(Indicies));
	i_buffer->Unlock();
	
	return true;
}

void RenderD3D()
{
	if (d3dDevice == NULL) return;

	RECT rc;
	GetClientRect(hwnd,&rc);
	
	d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); // draw backface 
	d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET| D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 120), 1.0f, 0);
	d3dDevice->BeginScene();

	d3dDevice->SetFVF(CUSTOMFVF);

	static float angle = 0.0f;
	struct constbuffer { XMMATRIX transform; };
	const constbuffer world = { XMMatrixMultiply(XMMatrixRotationY(angle),XMMatrixRotationX(angle)) };
	angle += (1.0f/57.29f);
	d3dDevice->SetTransform(D3DTS_WORLD,(D3DMATRIX*)& world);

	XMVECTOR eye = XMVectorSet(0.0f, 1.5f, 5.0f, 1.0f), lookat = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	const constbuffer view = {  XMMatrixLookAtLH(eye,lookat,up)} ;		
	d3dDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&view);

	const constbuffer proj = {  XMMatrixPerspectiveFovLH(45.f / 57.29f,(float)rc.right/rc.bottom,1.0f,30.f) };
	d3dDevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&proj);

	d3dDevice->SetStreamSource(0, v_buffer, 0, sizeof(CUSTOMVERTEX));
	d3dDevice->SetIndices(i_buffer);
	d3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,8,0,12);  // https://www.braynzarsoft.net/viewtutorial/q16390-07-dx9-index-buffers 

	const constbuffer world2 = { XMMatrixMultiply(XMMatrixRotationY(angle),XMMatrixTranslation(2.0f,0.0f,0.0f)) };
	d3dDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&world2);
	d3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,8,0,12);	

	d3dDevice->EndScene();
	d3dDevice->Present(NULL, NULL, NULL, NULL);
}

void ResizeD3D(){}

void EndD3D()
{
	if (i_buffer) i_buffer->Release();
	if (v_buffer) v_buffer->Release();
	if (d3dDevice) d3dDevice->Release();
	if (d3d) d3d->Release();
}