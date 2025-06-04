#include <windows.h>
#include <wrl.h>
#include <d3d11.h>  
#pragma comment(lib,"d3d11.lib") 
//#include <DirectXMath.h>

using namespace Microsoft::WRL;
//using namespace DirectX;

//struct SimpleVertex { XMFLOAT3 Pos; XMFLOAT4 Color; }; // vertex structure

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitD3D();  
void RenderD3D();

int width,height; 
BOOL ready = FALSE; // checks everything is ok
HWND hwnd = NULL;
RECT rect;

ComPtr<ID3D11Device> pD3DDevice;
ComPtr<IDXGISwapChain> pSwapChain;
ComPtr<ID3D11DeviceContext> pDeviceContext;
ComPtr<ID3D11RenderTargetView> pRenderTargetView;
ComPtr<ID3D11Texture2D> pBackBuffer;
D3D11_VIEWPORT vp;
HRESULT	hr;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{	
	WNDCLASS wc = { 0,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),
		LoadCursor(NULL,IDC_ARROW),(HBRUSH)COLOR_APPWORKSPACE + 1,NULL,L"d3d11" };
	RegisterClass(&wc);
		
	hwnd = CreateWindow(L"d3d11", L"Direct3D 11", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	ShowWindow(hwnd, show);
	GetClientRect(hwnd, &rect);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	if (SUCCEEDED(InitD3D())) ready = TRUE; 

	MSG msg = { 0 };
	while(true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (WM_QUIT == msg.message) break;
		else
			if (ready)
				RenderD3D(); 
	}	
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 300;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 300;
		return 0;
	case WM_SIZE:
		if (LOWORD(lParam) == 0) ready = FALSE; else ready = TRUE; // stop rendering when minimized
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			SendMessage(hwnd, WM_CLOSE, 0, 0); // escape key closes the window
			break;
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);		
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

HRESULT InitD3D()
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0; 
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL FeatureLevels = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL* pFeatureLevel = NULL;

	if (FAILED(hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		0, &FeatureLevels, 1, D3D11_SDK_VERSION, &sd, &pSwapChain, &pD3DDevice,
		pFeatureLevel, &pDeviceContext)))
	{
		MessageBox(hwnd, L"Failed to create D3D11 device", L"", MB_ICONERROR);
		return hr;
	}

	if (FAILED(hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)))
	{
		MessageBox(hwnd, L"Failed to GetBuffer", L"", MB_ICONERROR);
		return hr;
	}

	if (FAILED(hr = pD3DDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, &pRenderTargetView)))
	{
		MessageBox(hwnd, L"Failed to CreateRenderTargetView", L"", MB_ICONERROR);
		return hr;
	}

	pBackBuffer->Release();

	// bind the render target
	pDeviceContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), NULL);

	// the viewable area	
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pDeviceContext->RSSetViewports(1, &vp);

	return S_OK;
}

void RenderD3D()
{
	float clearColor[4] = { 0,0,1.0f,0 }; // RGBA
	pDeviceContext->ClearRenderTargetView(pRenderTargetView.Get(), clearColor);
	// drawing goes here
	pSwapChain->Present(0, 0); 
}
