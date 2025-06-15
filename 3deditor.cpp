#include <Windows.h>
#include <stdio.h> // FILE 
#include <strsafe.h> // StringCbPrintf 
#include <d3d10.h>
#pragma comment(lib,"d3d10.lib")
#include <DirectXMath.h>
using namespace DirectX;

HWND hwnd;
struct vec3 { float x, y, z; vec3(float x, float y, float z) : x(x), y(y), z(z) {} };
struct vec4 { float x, y, z, w; vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {} };
struct vertex { vec3 pos; vec4 color;	vertex(vec3 pos, vec4 color) : pos(pos), color(color) {} };

#define SHADER(x) #x // stringizing 
const char* shaderSource = SHADER(
matrix World;
matrix View;
matrix Projection;

struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VOut vs_shader(float4 position : POSITION, float4 color : COLOR)
{
	VOut output;

	position = mul(position, World);
	position = mul(position, View);
	output.position = mul(position, Projection);
	output.color = color;

	return output;
}

float4 ps_shader(float4 position : SV_POSITION, float4 color : COLOR) : SV_Target
{
	return color;
}

technique10 Render
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, vs_shader()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, ps_shader()));
	}
}
);

vec3 viewer = { 1.f,5.f,-10.f }, viewerlookat = { 0,0,0 }; // viewer and lookat
float angle = 0; // rotate viewer around y axis
int nModelVert = 0; // number of verticies loaded 

WCHAR txt[5000];
TCHAR szName[MAX_PATH]; // file name 

IDXGISwapChain* swapChain = NULL;
ID3D10Device* d3dDevice = NULL;
ID3D10RenderTargetView* renderTargetView = NULL;
ID3D10Blob* fxBlob = NULL;
ID3D10Blob* fxErrors = NULL;
ID3D10Effect* fxShader = NULL;
ID3D10Buffer* vertexBuffer = NULL;
ID3D10Buffer* vertexModelBuffer = NULL;
ID3D10InputLayout* vertexLayout = NULL;
ID3D10RasterizerState* rasterizerState = NULL;
ID3D10EffectTechnique* tech = NULL;
ID3D10EffectMatrixVariable* pViewMatrixEffectVariable = NULL;
ID3D10EffectMatrixVariable* pProjectionMatrixEffectVariable = NULL;
ID3D10EffectMatrixVariable* pWorldMatrixEffectVariable = NULL;
ID3D10Texture2D* backBuffer = NULL, * depthStencil = NULL;
ID3D10Buffer* indexBuffer = NULL;
ID3D10DepthStencilView* depthStencilView = NULL;

D3D10_INPUT_ELEMENT_DESC layout[] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0}
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitD3D();
void RenderD3D();
void EndD3D();
void ResizeD3D();
void LoadD3D();

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL, IDC_ARROW),(HBRUSH)GetStockObject(WHITE_BRUSH),NULL,L"3D Editor" };
	RegisterClass(&wc);

	hwnd = CreateWindowEx(WS_EX_ACCEPTFILES,L"3D Editor", L"3D Editor", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);

	MSG msg = {};

	InitD3D();
	
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) break;
		}
		else
		{
			if (!(GetWindowLong(hwnd, GWL_STYLE) & WS_MINIMIZE))
			{		
				static DWORD tick = GetTickCount();
				if (GetTickCount() < tick + 10) continue; // slow down to approx ~100fps
				tick = GetTickCount();
				RenderD3D();
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
	case WM_KEYDOWN:
	{
		if (LOWORD(wparam) == 'S') { if (viewer.z < -0.5) { viewer.x *= 0.95; viewer.y *= 0.95; viewer.z *= 0.95; } return 0; } // zoom out
		if (LOWORD(wparam) == 'W') { if (viewer.z > -20) { viewer.x *= 1.05; viewer.y *= 1.05; viewer.z *= 1.05; } return 0; } // zoom in
		if (LOWORD(wparam) == 'A') {
			XMVECTOR rot = XMVectorSet(viewer.x, viewer.y, viewer.z, 1.0f),left=XMVectorSet(0,0.01745,0,1); // rotate ~ 1 degrees 
			rot = XMVector3Rotate(rot, left);
			viewer.x = XMVectorGetX(rot);			
			viewer.y = XMVectorGetY(rot);			
			viewer.z = XMVectorGetZ(rot);
			return 0;
		}
		if (LOWORD(wparam) == 'D') {
			XMVECTOR rot = XMVectorSet(viewer.x, viewer.y, viewer.z, 1.0f), right = XMVectorSet(0, -0.01745, 0, 1);			
			rot = XMVector3Rotate(rot, right);
			viewer.x = XMVectorGetX(rot);			
			viewer.y = XMVectorGetY(rot);			
			viewer.z = XMVectorGetZ(rot);
			return 0;
		}
	}
	case WM_KEYUP:
	{
		if (LOWORD(wparam) == VK_ESCAPE) { PostQuitMessage(0); return 0; }
		return 0;
	}
	case WM_SIZE:
	{
		if (!(GetWindowLong(hwnd, GWL_STYLE) & WS_MINIMIZE)) ResizeD3D(); return 0;
	}
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMMI = (MINMAXINFO*)lparam;
		pMMI->ptMinTrackSize.x = 300;
		pMMI->ptMinTrackSize.y = 300;
		return 0;
	}
	case WM_DROPFILES: // https://github.com/gametutorials/tutorials/blob/master/Win32/Drag%20And%20Drop/Main.cpp
	{		
		HDROP hDrop = (HDROP)wparam;
		DragQueryFile(hDrop, 0, szName, MAX_PATH);
		DragFinish(hDrop);
		MessageBox(hwnd, szName,L"Load", NULL);
		LoadD3D();
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); return 0;
	}
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}

void InitD3D()
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = rc.right;
	swapChainDesc.BufferDesc.Height = rc.bottom;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = true;

	HRESULT hr = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &swapChain, &d3dDevice);
	if (FAILED(hr)) { MessageBox(hwnd, L"Create Device", NULL, NULL); PostQuitMessage(0); return; }

	swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&backBuffer);
	d3dDevice->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
	backBuffer->Release();

	D3D10_TEXTURE2D_DESC descDepth;

	descDepth.Width = rc.right;
	descDepth.Height = rc.bottom;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	if (FAILED(d3dDevice->CreateTexture2D(&descDepth, NULL, &depthStencil))) { MessageBox(hwnd, L"Create Depth", NULL, NULL); PostQuitMessage(0); return; }

	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	if (FAILED(d3dDevice->CreateDepthStencilView(depthStencil, &descDSV, &depthStencilView))) { MessageBox(hwnd, L"Create Depth View", NULL, NULL); PostQuitMessage(0); return; }

	d3dDevice->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Set viewport
	D3D10_VIEWPORT viewport;
	viewport.Width = rc.right;
	viewport.Height = rc.bottom;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	d3dDevice->RSSetViewports(1, &viewport);

	if (FAILED(D3D10CompileEffectFromMemory((void*)shaderSource, strlen(shaderSource), NULL, NULL, NULL, NULL, NULL, &fxBlob, &fxErrors)))
	{ MessageBox(hwnd, (WCHAR*)fxErrors->GetBufferPointer(), L"Compile from memory", NULL);	PostQuitMessage(0); return;	}

	if (FAILED(D3D10CreateEffectFromMemory(fxBlob->GetBufferPointer(), fxBlob->GetBufferSize(), NULL, d3dDevice, NULL, &fxShader)))
	{ MessageBox(hwnd, L"Create from memory", NULL, NULL);	PostQuitMessage(0); return;	}

	if (fxShader) tech = fxShader->GetTechniqueByName("Render"); else { MessageBox(hwnd, L"Render", NULL, NULL);	PostQuitMessage(0); return; }

	D3D10_PASS_DESC PassDesc;
	tech->GetPassByIndex(0)->GetDesc(&PassDesc);

	d3dDevice->CreateInputLayout(layout, 2, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &vertexLayout);
	d3dDevice->IASetInputLayout(vertexLayout);

	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(vertex) * 1000; 
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	d3dDevice->CreateBuffer(&bufferDesc, NULL, &vertexBuffer);

	UINT stride = sizeof(vertex);
	UINT offset = 0;
	d3dDevice->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	vertex* verticies;
	int nVertex = 0;
	vertexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&verticies);

	for (float n = -10; n <= 10; n++)
	{
		verticies[nVertex++] = vertex(vec3(n,0,-10), vec4(0.7, 0.7, 0.7, 1)); // grid lines 
		verticies[nVertex++] = vertex(vec3(n,0,10), vec4(0.7, 0.7, 0.7, 1));

		verticies[nVertex++] = vertex(vec3(-10,0,n), vec4(0.7, 0.7, 0.7, 1));
		verticies[nVertex++] = vertex(vec3(10,0,n), vec4(0.7, 0.7, 0.7, 1));
	}

	verticies[nVertex++] = vertex(vec3(0,0.005,0), vec4(1, 0, 0, 1)); // x red
	verticies[nVertex++] = vertex(vec3(1,0.005,0), vec4(1, 0, 0, 1));

	verticies[nVertex++] = vertex(vec3(0,0.005,0), vec4(0, 1, 0, 1)); // y green
	verticies[nVertex++] = vertex(vec3(0,1,0), vec4(0, 1, 0, 1));

	verticies[nVertex++] = vertex(vec3(0, 0.005, 0), vec4(0, 0, 1, 1)); // z blue
	verticies[nVertex++] = vertex(vec3(0, 0.005, 1), vec4(0, 0, 1, 1));
	// [84] is 21 lines * 2 points * 2 x lines, z lines   	
	vertexBuffer->Unmap();

	bufferDesc.ByteWidth = sizeof(unsigned int) * 1000;
	bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;

	d3dDevice->CreateBuffer(&bufferDesc, NULL, &indexBuffer);
	d3dDevice->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, offset);

	unsigned int* i = NULL; // not used yet , could make better file using indicies 
	indexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&i);
	i[0] = 0;
	i[1] = 1;
	i[2] = 2;
	i[3] = 0;
	i[4] = 2;
	i[5] = 3;
	i[6] = 4;
	indexBuffer->Unmap();

	D3D10_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.CullMode = D3D10_CULL_NONE;
	rasterizerDesc.FillMode = D3D10_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = false;
	rasterizerDesc.DepthBiasClamp = 0;
	rasterizerDesc.SlopeScaledDepthBias = 0;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = true;

	d3dDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	d3dDevice->RSSetState(rasterizerState);

	d3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST); // can use STRIP and 0xffffffff; 

	//create matrix effect pointers
	pViewMatrixEffectVariable = fxShader->GetVariableByName("View")->AsMatrix();
	pProjectionMatrixEffectVariable = fxShader->GetVariableByName("Projection")->AsMatrix();
	pWorldMatrixEffectVariable = fxShader->GetVariableByName("World")->AsMatrix();
}
void RenderD3D()
{
	RECT rc;
	GetClientRect(hwnd, &rc);
		
	XMMATRIX world = XMMatrixIdentity();
	pWorldMatrixEffectVariable->SetMatrix((float*)&world);

	XMVECTOR eye = XMVectorSet(viewer.x, viewer.y, viewer.z, 1.0f), lookat = XMVectorSet(viewerlookat.x, viewerlookat.y , viewerlookat.z, 1.0f), up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	XMMATRIX view = { XMMatrixLookAtLH(eye,lookat,up) };
	XMMATRIX proj = { XMMatrixPerspectiveFovLH(30.f / 57.29f,(float)rc.right / rc.bottom,0.01f,100.f) };

	pViewMatrixEffectVariable->SetMatrix((float*)&view);
	pProjectionMatrixEffectVariable->SetMatrix((float*)&proj);

	float c[4] = { 0.9f, 0.9f, 0.95f, 1 };
	d3dDevice->ClearRenderTargetView(renderTargetView, c);
	d3dDevice->ClearDepthStencilView(depthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

	D3D10_TECHNIQUE_DESC tDesc;
	tech->GetDesc(&tDesc);
	
	for (UINT p = 0; p < tDesc.Passes; p++)
	{
		tech->GetPassByIndex(p)->Apply(0);
		//d3dDevice->DrawIndexed(36, 0, 0);
		UINT stride = sizeof(vertex);	UINT offset = 0;	d3dDevice->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		d3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
		d3dDevice->Draw(90, 0); // draw grid 84 points, xyz lines 6 points
		d3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//d3dDevice->Draw(6, 90); // just for testing
		d3dDevice->IASetVertexBuffers(0, 1, &vertexModelBuffer, &stride, &offset);
		d3dDevice->Draw(nModelVert,0);
	}
	swapChain->Present(0, 0);
}

void ResizeD3D() // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#handling-window-resizing 
{
	if (swapChain)
	{
		d3dDevice->OMSetRenderTargets(0, 0, 0);
		renderTargetView->Release();
		swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

		swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&backBuffer);
		d3dDevice->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
		backBuffer->Release();

		// also depth stencil resize 

		RECT rc;
		GetClientRect(hwnd, &rc);

		D3D10_TEXTURE2D_DESC descDepth;

		descDepth.Width = rc.right;
		descDepth.Height = rc.bottom;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D32_FLOAT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D10_USAGE_DEFAULT;
		descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;

		depthStencil->Release();
		depthStencilView->Release();

		if (FAILED(d3dDevice->CreateTexture2D(&descDepth, NULL, &depthStencil))) { MessageBox(hwnd, L"Create Depth", NULL, NULL); PostQuitMessage(0); return; }

		D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
		descDSV.Format = descDepth.Format;
		descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;

		if (FAILED(d3dDevice->CreateDepthStencilView(depthStencil, &descDSV, &depthStencilView))) { MessageBox(hwnd, L"Create Depth View", NULL, NULL); PostQuitMessage(0); return; }

		//

		d3dDevice->OMSetRenderTargets(1, &renderTargetView, depthStencilView);		

		D3D10_VIEWPORT vp;
		vp.Width = rc.right;
		vp.Height = rc.bottom;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;

		d3dDevice->RSSetViewports(1, &vp);
	}
}

void LoadD3D()
{
	/*HANDLE hFile;
	hFile = CreateFile(szName, GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE) { MessageBox(hwnd, szName, L"Can't open file", MB_OK); return ; }
	char buffer[5000],szName2[5000];	
	DWORD toread = GetFileSize(hFile,NULL), read;
	BOOL errorfile;
	errorfile = ReadFile(hFile, &buffer, toread, &read, NULL);
	MessageBoxA(hwnd, buffer, "Read", NULL);
	CloseHandle(hFile);*/

	if (vertexModelBuffer) vertexModelBuffer->Release();

	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(vertex) * 1000;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	d3dDevice->CreateBuffer(&bufferDesc, NULL, &vertexModelBuffer);

	//UINT stride = sizeof(vertex);	UINT offset = 0;	d3dDevice->IASetVertexBuffers(0, 1, &vertexModelBuffer, &stride, &offset);

	char szNamec[5000];
	FILE* file;
	size_t conv;
	wcstombs_s(&conv, szNamec,MAX_PATH, szName, MAX_PATH);
	fopen_s(&file,szNamec, "r");
	int nAdd,nVertex=0;
	fscanf_s(file, "%d\n", &nAdd); // number of verticies to add 
	StringCbPrintfW(txt, 5000, L"%d Verticies\n", nAdd);
	nModelVert = nAdd; // for rendering later

	float x, y, z, r, g, b, a;
	vertex* verticies;
	vertexModelBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&verticies);

	for (int toAdd = 0; toAdd < nAdd; toAdd++)
	{
		fscanf_s(file, "%f,%f,%f,%f,%f,%f,%f", &x, &y, &z, &r, &g, &b, &a);
		StringCbPrintfW(txt, 5000, L"%s %0.1f %0.1f %0.1f\n", txt, x, y, z);
		verticies[nVertex++] = vertex(vec3(x, y, z), vec4(r, g, b, a));
	}	
	vertexModelBuffer->Unmap();
	MessageBox(hwnd, txt, L"Read", NULL);
	fclose(file);
}

void EndD3D()
{
	if (fxBlob) fxBlob->Release();
	if (fxErrors) fxErrors->Release();
	if (fxShader) fxShader->Release();

	if (depthStencilView) depthStencilView->Release();
	if (depthStencil) depthStencil->Release();
	if (indexBuffer) indexBuffer->Release();
	if (vertexBuffer) vertexBuffer->Release();
	if (vertexLayout) vertexLayout->Release();
	if (rasterizerState) rasterizerState->Release();
	if (renderTargetView) renderTargetView->Release();
	if (backBuffer) backBuffer->Release();
	if (swapChain) swapChain->Release();
	if (d3dDevice) d3dDevice->Release();
}