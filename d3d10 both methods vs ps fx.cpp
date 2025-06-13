#include <Windows.h>
#include <d3d10.h>
#pragma comment(lib,"d3d10.lib")

HWND hwnd;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitD3D();
void RenderD3D();
void EndD3D();

bool fx = true; // use vs,ps or fx

#define SHADER(x) #x // stringizing 
const char* shaderSource = SHADER(
	struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VOut vs_shader(float4 position : POSITION, float4 color : COLOR)
{
	VOut output;

	output.position = position;
	output.color = color;

	return output;
}

float4 ps_shader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
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

struct vec3 { float x, y, z; vec3(float x, float y, float z) : x(x), y(y), z(z) {} };
struct vec4 { float x, y, z, w; vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {} };
struct vertex { vec3 pos; vec4 color;	vertex(vec3 pos, vec4 color) : pos(pos), color(color) {} };

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show)
{
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW,WndProc,0,0,hInst,LoadIcon(NULL,IDI_APPLICATION),LoadCursor(NULL, IDC_ARROW),(HBRUSH)GetStockObject(WHITE_BRUSH),NULL,L"D3D10" };
	RegisterClass(&wc);

	hwnd = CreateWindow(L"D3D10", L"D3D10 Triangle", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
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

IDXGISwapChain* swapChain = NULL;
ID3D10Device* d3dDevice = NULL;
ID3D10Texture2D* backBuffer = NULL;
ID3D10RenderTargetView* renderTargetView = NULL;
ID3D10Blob* vertexBlob = NULL, * pixelBlob = NULL;
ID3D10VertexShader* vertexShader = NULL;
ID3D10PixelShader* pixelShader = NULL;
ID3D10Buffer* vertexBuffer = NULL;
ID3D10InputLayout* vertexLayout = NULL;
ID3D10RasterizerState* rasterizerState = NULL;


ID3D10Blob* fxBlob = NULL;
ID3D10Blob* errors = NULL;
ID3D10Effect* fxShader = NULL;
ID3D10EffectTechnique* tech = NULL;

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

	HRESULT hr = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &swapChain, &d3dDevice);
	if (FAILED(hr)) { MessageBox(hwnd, L"Create Device", NULL, NULL); PostQuitMessage(0); return; }

	swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&backBuffer);
	d3dDevice->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
	backBuffer->Release();
	d3dDevice->OMSetRenderTargets(1, &renderTargetView, NULL);

	// Set viewport
	D3D10_VIEWPORT viewport;
	viewport.Width = rc.right;
	viewport.Height = rc.bottom;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	d3dDevice->RSSetViewports(1, &viewport);

	// scene

	if (!fx)
	{
		D3D10CompileShader(shaderSource, strlen(shaderSource), NULL, NULL, NULL, "vs_shader", "vs_4_0", 0, &vertexBlob, NULL);
		d3dDevice->CreateVertexShader((DWORD*)vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &vertexShader);
		d3dDevice->VSSetShader(vertexShader);

		D3D10CompileShader(shaderSource, strlen(shaderSource), NULL, NULL, NULL, "ps_shader", "ps_4_0", 0, &pixelBlob, NULL);
		d3dDevice->CreatePixelShader((DWORD*)pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), &pixelShader);
		d3dDevice->PSSetShader(pixelShader);

		d3dDevice->CreateInputLayout(layout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &vertexLayout);
		d3dDevice->IASetInputLayout(vertexLayout);
	}
	else
	{
		if (FAILED(D3D10CompileEffectFromMemory((void*)shaderSource, strlen(shaderSource), NULL, NULL, NULL, NULL, NULL, &fxBlob, &errors)))
		{
			MessageBox(hwnd, (WCHAR*)errors->GetBufferPointer(), L"Compile from memory", NULL);	PostQuitMessage(0); return;
		}

		if (FAILED(D3D10CreateEffectFromMemory(fxBlob->GetBufferPointer(), fxBlob->GetBufferSize(), NULL, d3dDevice, NULL, &fxShader)))
		{
			MessageBox(hwnd, L"Create from memory", NULL, NULL);	PostQuitMessage(0); return;
		}
		if (fxShader) tech = fxShader->GetTechniqueByName("Render"); else { MessageBox(hwnd, L"Render", NULL, NULL);	PostQuitMessage(0); return; }

		D3D10_PASS_DESC PassDesc;
		tech->GetPassByIndex(0)->GetDesc(&PassDesc);

		d3dDevice->CreateInputLayout(layout, 2, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &vertexLayout);
		d3dDevice->IASetInputLayout(vertexLayout);
	}
	// above 

	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(vertex) * 6;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	d3dDevice->CreateBuffer(&bufferDesc, NULL, &vertexBuffer);

	UINT stride = sizeof(vertex);
	UINT offset = 0;
	d3dDevice->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	vertex* vertices;
	vertexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&vertices);
	if(!fx) vertices[0] = vertex(vec3(0, 0, 0), vec4(1, 0, 0, 1)); else vertices[0] = vertex(vec3(0, 0, 0), vec4(1, 1, 0, 1));
	vertices[1] = vertex(vec3(0.5, 1, 0), vec4(0, 1, 0, 1));
	vertices[2] = vertex(vec3(1, 0, 0), vec4(0, 0, 1, 1));

	vertices[3] = vertex(vec3(-1, -1, 0), vec4(1, 1, 0, 1));
	vertices[4] = vertex(vec3(-0.5, 0, 0), vec4(0, 1, 1, 1));
	vertices[5] = vertex(vec3(0, -1, 0), vec4(1, 0, 1, 1));
	vertexBuffer->Unmap();

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

	d3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void RenderD3D()
{
	float c[4] = { 0, 0, 0.5f, 1 };
	d3dDevice->ClearRenderTargetView(renderTargetView, c);
	if(!fx) d3dDevice->Draw(6, 0);
	else
	{
		D3D10_TECHNIQUE_DESC tDesc;
		tech->GetDesc(&tDesc); 
		for (UINT p = 0; p < tDesc.Passes; p++)
		{
			tech->GetPassByIndex(p)->Apply(0);
			d3dDevice->Draw(6, 0);
		}
	}
	swapChain->Present(0, 0);
}

void EndD3D()
{
	if (fxBlob) fxBlob->Release();
	if (errors) errors->Release();
	if (fxShader) fxShader->Release();
	
	if (pixelBlob) pixelBlob->Release();
	if (pixelShader) pixelShader->Release();
	if (vertexBlob) vertexBlob->Release();
	if (vertexBuffer) vertexBuffer->Release();
	if (vertexLayout) vertexLayout->Release();
	if (rasterizerState) rasterizerState->Release();
	if (renderTargetView) renderTargetView->Release();
	if (backBuffer) backBuffer->Release();
	if (swapChain) swapChain->Release();
	if (d3dDevice) d3dDevice->Release();
}