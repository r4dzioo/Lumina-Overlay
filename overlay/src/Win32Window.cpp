#include "Overlay/Win32Window.h"

#include "Overlay/Log.h"

#include <dwmapi.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <sstream>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

namespace overlay {

namespace {

std::wstring LastErrorText(const wchar_t* operation) {
    const DWORD error = GetLastError();
    std::wstringstream stream;
    stream << operation << L" failed with GetLastError=" << error;
    return stream.str();
}

bool IsLayered(HWND hwnd) {
    return (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED) != 0;
}

} // namespace

Win32Window::~Win32Window() {
    Destroy();
}

bool Win32Window::Create(HINSTANCE instance, int show_command, ResizeCallback resize_callback) {
    instance_ = instance;
    resize_callback_ = std::move(resize_callback);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &Win32Window::WindowProc;
    wc.hInstance = instance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LuminaOverlay.TransparentWindow";

    if (!RegisterClassExW(&wc)) {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            Log::Error(LastErrorText(L"RegisterClassExW"));
            return false;
        }
    }

    const int primary_width = GetSystemMetrics(SM_CXSCREEN);
    const int primary_height = GetSystemMetrics(SM_CYSCREEN);
    width_ = static_cast<UINT>(primary_width > 0 ? primary_width : 1280);
    height_ = static_cast<UINT>(primary_height > 0 ? primary_height : 720);

    struct WindowAttempt final {
        DWORD ex_style;
        DWORD style;
        int x;
        int y;
        int width;
        int height;
        const wchar_t* name;
    };

    const WindowAttempt attempts[] = {
        {WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, WS_POPUP, 0, 0, static_cast<int>(width_), static_cast<int>(height_), L"layered click-through overlay"},
        {WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, WS_POPUP, 0, 0, static_cast<int>(width_), static_cast<int>(height_), L"layered interactive overlay"},
        {WS_EX_TOPMOST | WS_EX_TOOLWINDOW, WS_POPUP, 0, 0, static_cast<int>(width_), static_cast<int>(height_), L"plain topmost overlay"},
        {0, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 960, 540, L"diagnostic window"},
    };

    for (const auto& attempt : attempts) {
        hwnd_ = CreateWindowExW(
            attempt.ex_style,
            wc.lpszClassName,
            L"Lumina Overlay",
            attempt.style,
            attempt.x,
            attempt.y,
            attempt.width,
            attempt.height,
            nullptr,
            nullptr,
            instance_,
            this);

        if (hwnd_) {
            Log::Info(std::wstring(L"Created overlay window using mode: ") + attempt.name);
            break;
        }

        Log::Warn(std::wstring(L"CreateWindowExW failed for mode: ") + attempt.name + L". " + LastErrorText(L"CreateWindowExW"));
    }

    if (!hwnd_) {
        Log::Error(L"All overlay window creation attempts failed.");
        return false;
    }

    if (IsLayered(hwnd_)) {
        MARGINS margins{-1};
        const HRESULT hr = DwmExtendFrameIntoClientArea(hwnd_, &margins);
        if (FAILED(hr)) {
            Log::Warn(L"DwmExtendFrameIntoClientArea failed.");
        }
    }

    SetOpacity(1.0f);
    ShowWindow(hwnd_, show_command);
    UpdateWindow(hwnd_);
    SetTopMost();
    return true;
}

void Win32Window::Destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool Win32Window::PumpMessages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            return false;
        }
    }
    return true;
}

void Win32Window::SetClickThrough(bool enabled) {
    if (!hwnd_ || click_through_ == enabled) {
        return;
    }

    click_through_ = enabled;
    LONG_PTR style = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    if (enabled) {
        style |= WS_EX_TRANSPARENT;
    } else {
        style &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, style);
}

void Win32Window::SetTopMost() {
    if (!hwnd_) {
        return;
    }

    SetWindowPos(
        hwnd_,
        HWND_TOPMOST,
        0,
        0,
        static_cast<int>(width_),
        static_cast<int>(height_),
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void Win32Window::SetOpacity(float opacity) {
    if (!hwnd_ || !IsLayered(hwnd_)) {
        return;
    }

    opacity = opacity < 0.05f ? 0.05f : (opacity > 1.0f ? 1.0f : opacity);
    const BYTE alpha = static_cast<BYTE>(opacity * 255.0f);
    if (!SetLayeredWindowAttributes(hwnd_, 0, alpha, LWA_ALPHA)) {
        Log::Warn(LastErrorText(L"SetLayeredWindowAttributes"));
    }
}

void Win32Window::ResizeToVirtualDesktop() {
    width_ = static_cast<UINT>(GetSystemMetrics(SM_CXSCREEN));
    height_ = static_cast<UINT>(GetSystemMetrics(SM_CYSCREEN));
}

LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)) {
        return true;
    }

    Win32Window* window = nullptr;
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        window = static_cast<Win32Window*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        if (window) {
            window->hwnd_ = hwnd;
        }
    } else {
        window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(message, wparam, lparam);
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT Win32Window::HandleMessage(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED) {
            width_ = LOWORD(lparam);
            height_ = HIWORD(lparam);
            if (resize_callback_) {
                resize_callback_(width_, height_);
            }
        }
        return 0;
    case WM_DISPLAYCHANGE:
        ResizeToVirtualDesktop();
        SetTopMost();
        if (resize_callback_) {
            resize_callback_(width_, height_);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd_, message, wparam, lparam);
    }
}

} // namespace overlay
