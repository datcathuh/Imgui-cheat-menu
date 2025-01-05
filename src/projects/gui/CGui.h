#pragma once

#include <d3d11.h>
#include <vector>

#include "CTextEditor.h"

enum ImFonts_ : int
{
	ImFont_Main,
	ImFont_Code
};

enum ImPages_ : int
{
	ImPage_Aimbot,
	ImPage_Visuals,
	ImPage_Lua,
	ImPage_Misc,
	ImPage_Configs,
	ImPage_Settings,

	ImPages_COUNT
};

enum eBones : int
{
	Bone_Head,
	Bone_LeftArm,
	Bone_RightArm,
	Bone_Belly,
	Bone_LeftLeg,
	Bone_RightLeg,

	Bones_COUNT
};

class CGui
{
private:
	int m_iCurrentPage;
	std::vector<const char*> m_Tabs;
	bool m_bSelectedBones[Bones_COUNT];

public:
	CGui();
	~CGui();

	bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* device_context);
	void Shutdown();
	void Render(const char* title, POINTS size, bool* b_open = 0);

	bool MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	ID3D11Device* GetDevice() { return m_pDevice; };
	ID3D11DeviceContext* GetDeviceContext() { return m_pDeviceContext; };
	HWND GetWindow() { return m_hWnd; };

private:
	HWND m_hWnd;
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;

private:
	CTextEditor* m_pTextEditor;
};
