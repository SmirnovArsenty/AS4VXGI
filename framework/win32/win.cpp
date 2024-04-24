#include <Windows.h>
#include <chrono>
#include <string>
#include <stdexcept>
#include <string>

#include "core/game.h"
#include "win.h"
#include "input.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_dx11.h>
#include <imgui/backends/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);


// static
LRESULT CALLBACK Win::WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    static int32_t old_x = 0;
    static int32_t old_y = 0;

    ImGui_ImplWin32_WndProcHandler(hWnd, message, wparam, lparam);

    switch (message)
    {
        case WM_GETMINMAXINFO:
        {   // need to handle it for fullscreen
            ((MINMAXINFO*)lparam)->ptMaxTrackSize.y =
                GetSystemMetrics(SM_CYMAXTRACK) +
                GetSystemMetrics(SM_CYCAPTION) +
                GetSystemMetrics(SM_CYMENU) +
                GetSystemMetrics(SM_CYBORDER) * 2;
            return 0;
        }
        case WM_INPUT:
        {
            Game::inst()->win().input()->handle_win_input(wparam, lparam);
            return 0;
        }
        case WM_DESTROY:
        {
            Game::inst()->set_destroy();
            Game::inst()->set_animating(false);
            return 0;
        }
        case WM_ENTERSIZEMOVE:
        {
            Game::inst()->set_animating(false);
            return 0;
        }
        case WM_EXITSIZEMOVE:
        {
            // OutputDebugString(std::to_string(wparam).c_str());
            Game::inst()->resize();
            Game::inst()->set_animating(true);
            return 0;
        }
        default:
        {
            return DefWindowProc(hWnd, message, wparam, lparam);
        }
    }
}

bool Win::initialize(uint32_t w, uint32_t h)
{
    constexpr char* app_name = "SGame";

    HINSTANCE hInst = GetModuleHandle(NULL);

    WNDCLASSEX wc;
    
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = app_name;
    wc.cbSize = sizeof(WNDCLASSEX);

    // Register the window class.
    RegisterClassEx(&wc);

    auto screenWidth = w;
    auto screenHeight = h;

    RECT windowRect = { 0, 0, static_cast<LONG>(screenWidth), static_cast<LONG>(screenHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_THICKFRAME; // WS_MINIMIZEBOX

    auto posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
    auto posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

    hWnd_ = CreateWindowEx(WS_EX_APPWINDOW, app_name, app_name,
        dwStyle,
        posX, posY,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hWnd_, SW_SHOW);
    SetForegroundWindow(hWnd_);
    SetFocus(hWnd_);

    ShowCursor(true);

    input_ = new Input(hWnd_);

    return true;
}

void Win::run()
{
    MSG msg{};
    while (PeekMessage(&msg, hWnd_, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Win::destroy()
{
    delete input_;
}

HWND Win::window() const
{
    return hWnd_;
}

Input* Win::input() const
{
    return input_;
}

float Win::screen_width() const
{
    RECT rc;
    GetWindowRect(Game::inst()->win().window(), &rc);
    return static_cast<float>(rc.right - rc.left);
}

float Win::screen_height() const
{
    RECT rc;
    GetWindowRect(Game::inst()->win().window(), &rc);
    return static_cast<float>(rc.bottom - rc.top);
}
