#pragma once

#include <windows.h>

#include <functional>
#include <string>

namespace overlay {

class Win32Window final {
public:
    using ResizeCallback = std::function<void(UINT, UINT)>;

    Win32Window() = default;
    ~Win32Window();

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    bool Create(HINSTANCE instance, int show_command, ResizeCallback resize_callback);
    void Destroy();
    bool PumpMessages();

    void SetClickThrough(bool enabled);
    void SetTopMost();
    void SetOpacity(float opacity);
    void ResizeToVirtualDesktop();

    HWND Handle() const { return hwnd_; }
    UINT Width() const { return width_; }
    UINT Height() const { return height_; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(UINT message, WPARAM wparam, LPARAM lparam);

    HWND hwnd_{nullptr};
    HINSTANCE instance_{nullptr};
    ResizeCallback resize_callback_;
    UINT width_{1920};
    UINT height_{1080};
    bool click_through_{false};
};

} // namespace overlay
