#include "CDirectX.h"

CDirectX::CDirectX()
{
	m_pDevice = 0;
	m_pDeviceContext = 0;
	m_pSwapChain = 0;
	m_pRenderTargetView = 0;
//	m_bSwapChainOccluded = false;
    m_bVSyncEnabled = false;
    m_hWnd = 0;
}

CDirectX::~CDirectX()
{
}

bool CDirectX::Initialize(bool vsync, HWND hwnd)
{
    m_bVSyncEnabled = vsync;
    m_hWnd = hwnd;

    if (!CreateDevice(hwnd))
    {
        CleanUpDevice();
        return false;
    }

	return true;
}

bool CDirectX::CreateDevice(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &m_pDevice, &featureLevel, &m_pDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &m_pDevice, &featureLevel, &m_pDeviceContext);
    if (res != S_OK)
        return false;

    if (!CreateRenderTarget())
        return false;

    return true;
}

void CDirectX::CleanUpDevice()
{
    CleanUpRenderTarget();

    if (m_pSwapChain) { 
        m_pSwapChain->Release(); 
        m_pSwapChain = nullptr; 
    }

    if (m_pDeviceContext) 
    { 
        m_pDeviceContext->Release(); 
        m_pDeviceContext = nullptr; 
    }

    if (m_pDevice) 
    { 
        m_pDevice->Release(); 
        m_pDevice = nullptr; 
    }
}

bool CDirectX::CreateRenderTarget()
{
    HRESULT result;
    ID3D11Texture2D* backBufferPtr;

    result = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferPtr));
    if (FAILED(result))
    {
        return false;
    }

    result = m_pDevice->CreateRenderTargetView(backBufferPtr, nullptr, &m_pRenderTargetView);
    if (FAILED(result))
    {
        return false;
    }

    backBufferPtr->Release();
    backBufferPtr = 0;

    return true;
}

void CDirectX::CleanUpRenderTarget()
{
    if (m_pRenderTargetView) 
    { 
        m_pRenderTargetView->Release(); 
        m_pRenderTargetView = nullptr; 
    }
}

void CDirectX::Shutdown()
{
    CleanUpDevice();
}


void CDirectX::BeginScene(float red, float green, float blue, float alpha)
{
	float color[4];

	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
}

void CDirectX::EndScene()
{
    if (m_bVSyncEnabled)
    {
        m_pSwapChain->Present(1, 0);
    }
    else
    {
        m_pSwapChain->Present(0, 0);
    }
}

void CDirectX::SetBuffersSize(UINT width, UINT height)
{
    if (width != 0 && height != 0)
    {
        CleanUpRenderTarget();
        m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        width = height = 0;
        CreateRenderTarget();
    }
}