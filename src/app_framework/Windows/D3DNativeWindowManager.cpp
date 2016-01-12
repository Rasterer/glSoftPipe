#include <cassert>
#include <cstring>

#include <windows.h>
#include <d3d11.h>
#include <D3Dcompiler.h>

#include "INativeWindowManager.h"
#include "glsp_defs.h"
#include "glsp_debug.h"


namespace glsp {

class D3DNativeWindowManager: public INativeWindowManager
{
public:
	D3DNativeWindowManager();
	virtual ~D3DNativeWindowManager();

	virtual bool NWMCreateWindow(int w, int h, const char *name);
	virtual void NWMDestroyWindow();
	virtual void GetWindowInfo(NWMWindowInfo *win_info);
	virtual bool DisplayFrame(NWMBufferToDisplay *buf);

private:
	bool InitD3DDevice();
	void DeinitD3DDevice();

	HWND                      mWND;
	int                       mWNDWidth;
	int                       mWNDHeight;
	const char               *mWNDName;

	D3D_FEATURE_LEVEL         mFeatureLevel;
	ID3D11Device             *mDevice;
	ID3D11DeviceContext      *mDeviceContext;
	IDXGISwapChain           *mSwapChain;
	ID3D11VertexShader       *mVS;
	ID3D11PixelShader        *mPS;
	ID3D11RenderTargetView   *mRenderTargetView;
	ID3D11ShaderResourceView *mShaderResourceView;
	ID3D11InputLayout        *mInputLayout;
	ID3D11Buffer             *mVertexBuffer;
	ID3D11SamplerState       *mSamplerState;
	ID3D11Texture2D          *mTexture;
};


static const char vs_source[] =
	"struct VS_INPUT \n"
	"{ \n"
	"	float2 pos : POSITION; \n"
	"}; \n"
	"struct VS_OUTPUT \n"
	"{ \n"
	"	float4 pos : SV_POSITION; \n"
	"	float2 tex : TEXCOORD0; \n"
	"}; \n"
	"void VSMain(in VS_INPUT input, out VS_OUTPUT output) \n"
	"{ \n"
	"	output.pos = float4(input.pos, 0.0f ,1.0f); \n"
	"	output.tex = float2(float2(input.pos.x, -input.pos.y) * 0.5f + 0.5f); \n"
	"} \n";

static const char ps_source[] =
	"struct PS_INPUT \n"
	"{ \n"
	"	float4 pos : SV_POSITION; \n"
	"	float2 tex : TEXCOORD0; \n"
	"}; \n"
	"Texture2D tx : register(t0); \n"
	"SamplerState ss : register(s0); \n"
	"float4 PSMain(in PS_INPUT input) : SV_Target \n"
	"{ \n"
	"	return tx.Sample(ss, input.tex); \n"
	"} \n";


static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
    case WM_PAINT:
        hdc = ::BeginPaint( hWnd, &ps );
        ::EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        ::PostQuitMessage( 0 );
        break;

    default:
        return ::DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

D3DNativeWindowManager::D3DNativeWindowManager():
	mWND(nullptr),
	mWNDWidth(0),
	mWNDHeight(0),
	mWNDName(nullptr),
	mDevice(nullptr),
	mFeatureLevel(D3D_FEATURE_LEVEL_11_0),
	mDeviceContext(nullptr),
	mSwapChain(nullptr),
	mVS(nullptr),
	mPS(nullptr),
	mRenderTargetView(nullptr),
	mShaderResourceView(nullptr),
	mInputLayout(nullptr),
	mVertexBuffer(nullptr),
	mSamplerState(nullptr),
	mTexture(nullptr)
{
}

D3DNativeWindowManager::~D3DNativeWindowManager()
{
	NWMDestroyWindow();
}

bool D3DNativeWindowManager::NWMCreateWindow(int w, int h, const char *name)
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	::WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = "GlspApp";
	wcex.hIconSm = ::LoadIcon(nullptr, IDI_APPLICATION);
	if (!::RegisterClassEx(&wcex))
		return false;

	mWNDWidth  = w;
	mWNDHeight = h;
	mWNDName   = name;

	::RECT rc = {0, 0, w, h};
	::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	mWND = ::CreateWindow("GlspApp", name,
						WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
						CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
						nullptr);
    if (!mWND)
        return false;

    ::ShowWindow(mWND, SW_SHOW);

    return InitD3DDevice();
}

bool D3DNativeWindowManager::InitD3DDevice()
{
	const ::D3D_FEATURE_LEVEL feature_levels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

	::DXGI_SWAP_CHAIN_DESC sd = {0};

	sd.BufferCount = 1;
	sd.BufferDesc.Width  = mWNDWidth;
	sd.BufferDesc.Height = mWNDHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = mWND;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	//sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
					nullptr, 0, feature_levels, ARRAY_SIZE(feature_levels),
					D3D11_SDK_VERSION, &sd, &mSwapChain, &mDevice, &mFeatureLevel, &mDeviceContext);

	if (FAILED(hr))
	{
		return false;
	}

	::ID3D11Texture2D* back_buffer;
	mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
	mDevice->CreateRenderTargetView(back_buffer, nullptr, &mRenderTargetView);
	back_buffer->Release();
	mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);

	::D3D11_VIEWPORT vp;
	vp.Width  = (FLOAT)mWNDWidth;
	vp.Height = (FLOAT)mWNDHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	mDeviceContext->RSSetViewports(1, &vp);

	::ID3DBlob *vs_blob  = nullptr;
	::ID3DBlob *err_blob = nullptr;
	::D3DCompile(vs_source, sizeof(vs_source), nullptr, nullptr, nullptr, "VSMain", "vs_4_0", 0, 0, &vs_blob, &err_blob);

	if (err_blob)
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "vertex shader compilation failed: %s\n", err_blob->GetBufferPointer());
		return false;
	}

	mDevice->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &mVS);
	mDeviceContext->VSSetShader(mVS, nullptr, 0);

	struct Point2
	{
		float x;
		float y;
	} vert_buf[4] = {{-1.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, -1.0f}, {1.0f, -1.0f}};

	::D3D11_BUFFER_DESC bd;
	bd.ByteWidth = sizeof(vert_buf);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	::D3D11_SUBRESOURCE_DATA init_data;
	init_data.pSysMem = vert_buf;
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	mDevice->CreateBuffer(&bd, &init_data, &mVertexBuffer);

	const UINT stride = sizeof(Point2);
	const UINT offset = 0;
	mDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);

	::D3D11_INPUT_ELEMENT_DESC layout;
	layout.SemanticName = "POSITION";
	layout.SemanticIndex = 0;
	layout.Format = DXGI_FORMAT_R32G32_FLOAT;
	layout.InputSlot = 0;
	layout.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	layout.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layout.InstanceDataStepRate = 0;

	mDevice->CreateInputLayout(&layout, 1, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &mInputLayout);
	vs_blob->Release();

	mDeviceContext->IASetInputLayout(mInputLayout);
	mDeviceContext->IASetPrimitiveTopology(::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	::ID3DBlob *ps_blob = nullptr;
	err_blob  = nullptr;
	::D3DCompile(ps_source, sizeof(ps_source), nullptr, nullptr, nullptr, "PSMain", "ps_4_0", 0, 0, &ps_blob, &err_blob);

	if (err_blob)
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "pixel shader compilation failed: %s\n", err_blob->GetBufferPointer());
		return false;
	}

	mDevice->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &mPS);
	ps_blob->Release();

	mDeviceContext->PSSetShader(mPS, NULL, 0);

	::D3D11_SAMPLER_DESC sampler;
	sampler.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler.BorderColor[0] = 0.0f;
	sampler.BorderColor[1] = 0.0f;
	sampler.BorderColor[2] = 0.0f;
	sampler.BorderColor[3] = 0.0f;
	sampler.MinLOD = 0;
	sampler.MaxLOD = 0;
	mDevice->CreateSamplerState(&sampler, &mSamplerState);
	mDeviceContext->PSSetSamplers(0, 1, &mSamplerState);

	::D3D11_TEXTURE2D_DESC td;
	td.Width = (UINT)mWNDWidth;
	td.Height = (UINT)mWNDHeight;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DYNAMIC;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	td.MiscFlags = 0;
	hr = mDevice->CreateTexture2D(&td, NULL, &mTexture);
	if (FAILED(hr))
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "CreateTexture2D fail %#lx\n", hr);
	}

	::D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.MipLevels = 1;
	mDevice->CreateShaderResourceView(mTexture, &srvd, &mShaderResourceView);
	mDeviceContext->PSSetShaderResources(0, 1, &mShaderResourceView);
}

void D3DNativeWindowManager::NWMDestroyWindow()
{
	DeinitD3DDevice();
}

void D3DNativeWindowManager::DeinitD3DDevice()
{
	if (mDeviceContext)        mDeviceContext->ClearState();

	if (mShaderResourceView)   mShaderResourceView->Release();
	if (mRenderTargetView)     mRenderTargetView->Release();
	if (mTexture)              mTexture->Release();
	if (mSamplerState)         mSamplerState->Release();
	if (mVertexBuffer)         mVertexBuffer->Release();
	if (mInputLayout)          mInputLayout->Release();
	if (mPS)                   mPS->Release();
	if (mVS)                   mVS->Release();
	if (mSwapChain)            mSwapChain->Release();
	if (mDeviceContext)        mDeviceContext->Release();
	if (mDevice)               mDeviceContext->Release();
}

void D3DNativeWindowManager::GetWindowInfo(NWMWindowInfo *win_info)
{
	win_info->width  = mWNDWidth;
	win_info->height = mWNDHeight;
	win_info->format = DXGI_FORMAT_R8G8B8A8_UNORM;
}

bool D3DNativeWindowManager::DisplayFrame(NWMBufferToDisplay *buf)
{
	// TODO: check format as well
	if (buf->width != mWNDWidth || buf->height != mWNDHeight)
		GLSP_DPF(GLSP_DPF_LEVEL_WARNING, "buffer size mismatch\n");

	::D3D11_MAPPED_SUBRESOURCE map_sr;
	HRESULT hr = mDeviceContext->Map(mTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &map_sr);
	if (FAILED(hr))
	{
		GLSP_DPF(GLSP_DPF_LEVEL_ERROR, "map texture fail %#lx\n", hr);
	}

	const int size_per_line = buf->width * 4;
	const int size = size_per_line * buf->height;
	assert(map_sr.RowPitch >= size_per_line);

	if (map_sr.RowPitch == buf->width)
	{
		std::memcpy(map_sr.pData, buf->addr, size);
	}
	else
	{
		for (int i = 0; i < buf->height; ++i)
		{
			std::memcpy(map_sr.pData, buf->addr, size_per_line);

			map_sr.pData = (char *)map_sr.pData + map_sr.RowPitch;
			buf->addr = (char *)buf->addr + size_per_line;
		}
	}

	mDeviceContext->Unmap(mTexture, 0);
	mDeviceContext->Draw(4, 0);
	mSwapChain->Present(0, 0);

	return true;
}

INativeWindowManager* CreateNativeWindowManager()
{
	return new D3DNativeWindowManager();
}

} // namespace glsp
