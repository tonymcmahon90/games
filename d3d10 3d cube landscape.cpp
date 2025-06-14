#include <Windows.h>
#include <d3d10.h>
#pragma comment(lib,"d3d10.lib")
#include <DirectXMath.h>
using namespace DirectX;

HWND hwnd;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitD3D();
void RenderD3D();
void EndD3D();
void Matrix();

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
		output.position = mul(position,Projection); 
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

struct _vert{ XMFLOAT3 pos; XMFLOAT4 col; };

struct vec3 { float x, y, z; vec3(float x, float y, float z) : x(x), y(y), z(z) {} };
struct vec4 { float x, y, z, w; vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {} };
struct vertex { vec3 pos; vec4 color;	vertex(vec3 pos, vec4 color) : pos(pos), color(color) {} };
/*
_vert VV[] = {
	{XMFLOAT3(0.0f,0.5f,0.0f),XMFLOAT4(1,0,0,1)},
	{XMFLOAT3(0.5f,-0.5f,0.0f),XMFLOAT4(1,1,0,1)},
	{XMFLOAT3(-0.5f,-0.5f,0.0f),XMFLOAT4(1,0,1,1)},

	{XMFLOAT3(-1.0f,-1.0f,0.0f),XMFLOAT4(1,0,1,1)},
	{XMFLOAT3(-1.0f,0.0f,0.0f),XMFLOAT4(1,1,1,1)},
	{XMFLOAT3(0.0f,0.0f,0.0f),XMFLOAT4(0,1,0,1)},
};*/

WCHAR txt[1000];

vec3 player = { 0,1.5f,-5.f }; // player position 

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL, IDC_ARROW),(HBRUSH)GetStockObject(WHITE_BRUSH),NULL,L"D3D10"};
	RegisterClass(&wc);

	hwnd = CreateWindow(L"D3D10", L"D3D10 Triangle", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ShowWindow(hwnd, show);
	
	MSG msg = {};

	InitD3D();
	SYSTEMTIME tt,nt;
	GetSystemTime(&tt);

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
			if (!(GetWindowLong(hwnd,GWL_STYLE) & WS_MINIMIZE))
			{
				RenderD3D();
				static int n = 0;
				GetSystemTime(&nt);
				if (tt.wSecond != nt.wSecond)
				{
					wsprintf(txt, L"%d fps", n);
					SetWindowText(hwnd, txt);
					n = 0;
					tt.wSecond = nt.wSecond;
				}
				n++;
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
			if (LOWORD(wparam) == 'W') { player.z += 0.1f; return 0; }
			if (LOWORD(wparam) == 'S') { player.z -= 0.1f; return 0; }
			if (LOWORD(wparam) == 'A') { player.x -= 0.1f; return 0; }
			if (LOWORD(wparam) == 'D') { player.x += 0.1f; return 0; }
			return 0;
		}
		case WM_KEYUP:
		{	
			
			if (LOWORD(wparam) == VK_ESCAPE) { PostQuitMessage(0); return 0; }
			return 0;
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

IDXGISwapChain* swapChain=NULL;
ID3D10Device* d3dDevice=NULL;
ID3D10RenderTargetView* renderTargetView=NULL;
ID3D10Blob* fxBlob=NULL;
ID3D10Blob* fxErrors = NULL;
ID3D10Effect* fxShader=NULL;
ID3D10Buffer* vertexBuffer=NULL;
ID3D10InputLayout* vertexLayout=NULL;
ID3D10RasterizerState* rasterizerState=NULL;
ID3D10EffectTechnique* tech = NULL;
ID3D10EffectMatrixVariable* pViewMatrixEffectVariable = NULL;
ID3D10EffectMatrixVariable* pProjectionMatrixEffectVariable = NULL;
ID3D10EffectMatrixVariable* pWorldMatrixEffectVariable = NULL;
ID3D10Texture2D* backBuffer = NULL,*depthStencil=NULL;
ID3D10Buffer* indexBuffer = NULL;
ID3D10DepthStencilView* depthStencilView = NULL;

D3D10_INPUT_ELEMENT_DESC layout[] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0}
};

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

	HRESULT hr=D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &swapChain, &d3dDevice);
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

	if(FAILED(d3dDevice->CreateTexture2D(&descDepth,NULL,&depthStencil))){ MessageBox(hwnd, L"Create Depth", NULL, NULL); PostQuitMessage(0); return; }

	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	if(FAILED(d3dDevice->CreateDepthStencilView(depthStencil,&descDSV,&depthStencilView))) { MessageBox(hwnd, L"Create Depth View", NULL, NULL); PostQuitMessage(0); return; }

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
	{	MessageBox(hwnd, (WCHAR*)fxErrors->GetBufferPointer(), L"Compile from memory", NULL);	PostQuitMessage(0); return;	}

	if (FAILED(D3D10CreateEffectFromMemory(fxBlob->GetBufferPointer(), fxBlob->GetBufferSize(), NULL, d3dDevice, NULL, &fxShader)))
	{	MessageBox(hwnd, L"Create from memory", NULL, NULL);	PostQuitMessage(0); return;	}	
	
	if(fxShader) tech = fxShader->GetTechniqueByName("Render"); else { MessageBox(hwnd, L"Render", NULL, NULL);	PostQuitMessage(0); return; }
	
	D3D10_PASS_DESC PassDesc;
	tech->GetPassByIndex(0)->GetDesc(&PassDesc);

	d3dDevice->CreateInputLayout(layout, 2, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &vertexLayout);
	d3dDevice->IASetInputLayout(vertexLayout);
	
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(_vert)*60; // was _vert 
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	d3dDevice->CreateBuffer(&bufferDesc,NULL, &vertexBuffer);

	UINT stride = sizeof(vertex);
	UINT offset = 0;
	d3dDevice->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	vertex* verticies;
	vertexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&verticies);
	
	verticies[0] = vertex(vec3(-1,0.2,1), vec4(0, 0.6, 0, 1));
	verticies[1] = vertex(vec3(1,0.2,1), vec4(0, 0.6, 0, 1));
	verticies[2] = vertex(vec3(1,0.2,-1), vec4(0, 0.6, 0, 1));
	verticies[3] = vertex(vec3(-1,0.2,-1), vec4(0, 0.6, 0, 1));

	verticies[4] = vertex(vec3(-1,-0.2,1), vec4(0.4, 0.2, 0, 1));
	verticies[5] = vertex(vec3(1,-0.2,1), vec4(0.4, 0.2, 0, 1));
	verticies[6] = vertex(vec3(1,-0.2,-1), vec4(0.4, 0.2, 0, 1));
	verticies[7] = vertex(vec3(-1,-0.2,-1), vec4(0.4, 0.2, 0, 1));

	//for (int n = 0; n < 6; n++) verticies[n] = VV[n];
	vertexBuffer->Unmap();

	bufferDesc.ByteWidth = sizeof(unsigned int) * 100;
	bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;

	d3dDevice->CreateBuffer(&bufferDesc, NULL, &indexBuffer);
	d3dDevice->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT,offset);

	unsigned int* i = NULL;
	indexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&i);
	i[0] = 0;
	i[1] = 1;
	i[2] = 2;
	i[3] = 0;
	i[4] = 2;
	i[5] = 3;
	i[6] = 4;
	i[7] = 5;
	i[8] = 6;
	i[9] = 4;
	i[10] = 6;
	i[11] = 7;
	i[12] = 0;
	i[13] = 1;
	i[14] = 5;
	i[15] = 0;
	i[16] = 5;
	i[17] = 4;
	i[18] = 2;
	i[19] = 1;
	i[20] = 5;
	i[21] = 2;
	i[22] = 5;
	i[23] = 6;
	i[24] = 3;
	i[25] = 2;
	i[26] = 6;
	i[27] = 3;
	i[28] = 6;
	i[29] = 7;
	i[30] = 3;
	i[31] = 0;
	i[32] = 4;
	i[33] = 3;
	i[34] = 4;
	i[35] = 7;

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

	d3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // can use STRIP and 0xffffffff; 

	// same as just two triangles 

	//create matrix effect pointers
	pViewMatrixEffectVariable = fxShader->GetVariableByName("View")->AsMatrix();
	pProjectionMatrixEffectVariable = fxShader->GetVariableByName("Projection")->AsMatrix();
	pWorldMatrixEffectVariable = fxShader->GetVariableByName("World")->AsMatrix();

	Matrix();
}

void Matrix()
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	static float angle = 0.0f; // radians 
	XMMATRIX world = { XMMatrixRotationZ(angle) };
	XMVECTOR eye = XMVectorSet(player.x,player.y, player.z, 1.0f), lookat = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	XMMATRIX view = { XMMatrixLookAtLH(eye,lookat,up) };
	XMMATRIX proj = { XMMatrixPerspectiveFovLH(30.f / 57.29f,(float)rc.right / rc.bottom,1.0f,50.f) };

	pWorldMatrixEffectVariable->SetMatrix((float*)&world);
	pViewMatrixEffectVariable->SetMatrix((float*)&view);
	pProjectionMatrixEffectVariable->SetMatrix((float*)&proj);
}	

void RenderD3D()
{		
	RECT rc;
	GetClientRect(hwnd, &rc);

	//static float angle = 0.0f;	XMMATRIX world = { XMMatrixMultiply(XMMatrixRotationX(angle),XMMatrixRotationY(angle)) };
	XMMATRIX world = XMMatrixIdentity();
	//angle += (0.01f / 57.29f);
	pWorldMatrixEffectVariable->SetMatrix((float*)&world);

	XMVECTOR eye = XMVectorSet(player.x, player.y, player.z, 1.0f), lookat = XMVectorSet(player.x, player.y-1.0f, player.z+5.0f, 1.0f), up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	XMMATRIX view = { XMMatrixLookAtLH(eye,lookat,up) };
	XMMATRIX proj = { XMMatrixPerspectiveFovLH(30.f / 57.29f,(float)rc.right / rc.bottom,1.0f,50.f) };

	pViewMatrixEffectVariable->SetMatrix((float*)&view);
	pProjectionMatrixEffectVariable->SetMatrix((float*)&proj);

	float c[4] = { 0, 0, 0.4f, 1 };
	d3dDevice->ClearRenderTargetView(renderTargetView, c);
	d3dDevice->ClearDepthStencilView(depthStencilView, D3D10_CLEAR_DEPTH,1.0f, 0);

	D3D10_TECHNIQUE_DESC tDesc;
	tech->GetDesc(&tDesc);

	for(float z=-20.f;z<=20.f;z+=2.2f)
	for (float x = -20.f; x <= 20.f; x += 2.2f)
	{
		world = XMMatrixTranslation(x, 0, z);
		pWorldMatrixEffectVariable->SetMatrix((float*)&world);

		for (UINT p = 0; p < tDesc.Passes; p++)
		{
			tech->GetPassByIndex(p)->Apply(0);
			d3dDevice->DrawIndexed(36, 0, 0);
		}
	}
	swapChain->Present(0, 0);
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