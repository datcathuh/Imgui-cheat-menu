#pragma once

#include <Windows.h>

class CAppWnd
{
public:
	CAppWnd();
	~CAppWnd();

public:
	bool Create(WNDPROC wnd_proc, HINSTANCE instance, const char* title, const char* wnd_class, POINTS position, POINTS size);
	void SetState(int cmd_show);
	void Shutdown();

public:
	HWND GetWindow() { return m_hWnd; };
	WNDCLASSA GetWindowClass() { return m_WndClass; };

private:
	HWND m_hWnd;
	WNDCLASSA m_WndClass;
};