#pragma once

#include "Overlay/Config/OverlayConfig.h"

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <chrono>

namespace overlay {

class Dx11Renderer final {
public:
    Dx11Renderer() = default;
    ~Dx11Renderer();

    Dx11Renderer(const Dx11Renderer&) = delete;
    Dx11Renderer& operator=(const Dx11Renderer&) = delete;

    bool Initialize(HWND hwnd, const Config::Theme& theme);
    void Shutdown();
    void Resize(UINT width, UINT height);

    void BeginFrame();
    void EndFrame(bool performance_mode);
    void ApplyTheme(const Config::Theme& theme);
    void SetUiScale(float scale);

    ID3D11Device* Device() const { return device_.Get(); }
    ID3D11DeviceContext* Context() const { return context_.Get(); }

private:
    bool CreateDeviceAndSwapChain(HWND hwnd);
    bool CreateRenderTarget();
    void ReleaseRenderTarget();
    void RebuildFontAtlas(float scale);

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_;

    HWND hwnd_{nullptr};
    bool initialized_{false};
    float ui_scale_{1.0f};
    std::string font_path_;
    std::chrono::steady_clock::time_point last_present_{};
};

} // namespace overlay

