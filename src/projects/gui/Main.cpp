#include "CAppWnd.h"
#include "CDirectX.h"
#include "CGui.h"
#include "Globals.h"

CAppWnd* g_pAppWnd = nullptr;
CDirectX* g_pDirectX = nullptr;
CGui* g_pGui = nullptr;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static POINTS g_WindowPosition;

    if (g_pGui->MsgProc(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }

    switch (uMsg)
    {
        case WM_SIZE:
        {
            if (wParam == SIZE_MINIMIZED)
                return 0;

            g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
            g_ResizeHeight = (UINT)HIWORD(lParam);

            return 0;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        default:
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{


    int iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_pAppWnd = new CAppWnd();
    if (!g_pAppWnd->Create(WndProc, hInstance, APP_NAME, APP_WND_CLASS, { static_cast<short>(iScreenWidth / 2 - APP_WND_WIDTH / 2), static_cast<short>(iScreenHeight / 2 - APP_WND_HEIGHT / 2) }, APP_WND_SIZE))
    {
        MessageBox(NULL, "Could not create window", "Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    g_pDirectX = new CDirectX();
    g_pDirectX->Initialize(VSYNC_ENABLED, g_pAppWnd->GetWindow());
    g_pGui = new CGui();
    g_pGui->Initialize(g_pAppWnd->GetWindow(), g_pDirectX->GetDevice(), g_pDirectX->GetDeviceContext());
    g_pAppWnd->SetState(SW_SHOWNORMAL);

    bool done = false;
    bool running = true;
    while (!done)
    {
        done = !running;

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        g_pDirectX->SetBuffersSize(g_ResizeWidth, g_ResizeHeight);
        //g_pDirectX->BeginScene(0.0120f, 0.514f, 0.529f, 1.000f);
        g_pDirectX->BeginScene(0.0f, 0.0f, 0.0f, 1.0f);
        g_pGui->Render(APP_NAME, APP_WND_SIZE, &running);
        g_pDirectX->EndScene();
    }

    g_pGui->Shutdown();
    g_pDirectX->Shutdown();
    g_pAppWnd->Shutdown();

    delete g_pGui;
    delete g_pDirectX;
    delete g_pAppWnd;

    return EXIT_SUCCESS;
}
