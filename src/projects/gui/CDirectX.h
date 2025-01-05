#pragma once

#pragma warning(disable: 4005) // Disable warning C4005 (macro redefinition)
#include <d3d11.h>
#include <directxsdk/d3dx11.h>
#include <directxmath.h>
#pragma warning(default: 4005) // Re-enable warning C4005

using namespace DirectX;

class CDirectX
{
public:
	CDirectX();
	~CDirectX();

	bool Initialize(bool vsync, HWND hwnd);
	void Shutdown();
	
	void BeginScene(float red, float green, float blue, float alpha);
	void EndScene();

	bool CreateDevice(HWND hwnd);
	void CleanUpDevice();
	bool CreateRenderTarget();
	void CleanUpRenderTarget();
	void SetBuffersSize(UINT width, UINT height);

	ID3D11Device* GetDevice() { return m_pDevice; };
	ID3D11DeviceContext* GetDeviceContext() { return m_pDeviceContext; };
	HWND GetWindow() { return m_hWnd; };

private:
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	IDXGISwapChain* m_pSwapChain;
	ID3D11RenderTargetView* m_pRenderTargetView;
//	bool m_bSwapChainOccluded;
	HWND m_hWnd;

private:
	bool m_bVSyncEnabled;
};
