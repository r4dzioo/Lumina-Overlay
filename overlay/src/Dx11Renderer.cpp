#include "Overlay/Dx11Renderer.h"

#include "Overlay/Log.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <iomanip>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace overlay {

namespace {

std::wstring HResultText(HRESULT hr) {
    std::wstringstream stream;
    stream << L"0x" << std::hex << std::uppercase << static_cast<unsigned long>(hr);
    return stream.str();
}

std::string Narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }

    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

std::string SystemFontPath(const wchar_t* file_name) {
    std::wstring windows_dir(MAX_PATH, L'\0');
    const UINT length = GetWindowsDirectoryW(windows_dir.data(), static_cast<UINT>(windows_dir.size()));
    if (length == 0 || length >= windows_dir.size()) {
        return {};
    }

    windows_dir.resize(length);
    return Narrow(windows_dir + L"\\Fonts\\" + file_name);
}

} // namespace

Dx11Renderer::~Dx11Renderer() {
    Shutdown();
}

bool Dx11Renderer::Initialize(HWND hwnd, const Config::Theme& theme) {
    hwnd_ = hwnd;
    if (!CreateDeviceAndSwapChain(hwnd_)) {
        return false;
    }
    if (!CreateRenderTarget()) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    font_path_ = SystemFontPath(L"segoeui.ttf");
    RebuildFontAtlas(1.0f);

    ApplyTheme(theme);

    if (!ImGui_ImplWin32_Init(hwnd_) || !ImGui_ImplDX11_Init(device_.Get(), context_.Get())) {
        return false;
    }

    initialized_ = true;
    return true;
}

void Dx11Renderer::Shutdown() {
    if (initialized_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        initialized_ = false;
    }
    ReleaseRenderTarget();
    swap_chain_.Reset();
    context_.Reset();
    device_.Reset();
}

void Dx11Renderer::Resize(UINT width, UINT height) {
    if (!swap_chain_ || width == 0 || height == 0) {
        return;
    }

    ReleaseRenderTarget();
    swap_chain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void Dx11Renderer::BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Dx11Renderer::EndFrame(bool performance_mode) {
    ImGui::Render();

    constexpr float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context_->OMSetRenderTargets(1, render_target_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(render_target_.Get(), clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    const UINT sync_interval = performance_mode ? 0U : 1U;
    swap_chain_->Present(sync_interval, 0);
    last_present_ = std::chrono::steady_clock::now();
}

void Dx11Renderer::SetUiScale(float scale) {
    scale = std::clamp(scale, 0.75f, 1.75f);
    if (std::abs(scale - ui_scale_) < 0.025f) {
        return;
    }

    // Keep paddings, controls, and rounding proportional to the rasterized font.
    ImGui::GetStyle().ScaleAllSizes(scale / ui_scale_);
    if (initialized_) {
        ImGui_ImplDX11_InvalidateDeviceObjects();
    }
    RebuildFontAtlas(scale);
    if (initialized_) {
        ImGui_ImplDX11_CreateDeviceObjects();
    }
}

void Dx11Renderer::RebuildFontAtlas(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFontConfig font_config{};
    font_config.OversampleH = 3;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;
    const float font_size = 16.0f * scale;

    ImFont* font = nullptr;
    if (!font_path_.empty()) {
        font = io.Fonts->AddFontFromFileTTF(font_path_.c_str(), font_size, &font_config);
    }
    if (!font) {
        font = io.Fonts->AddFontDefault();
        Log::Warn(L"Segoe UI is unavailable; using the Dear ImGui fallback font.");
    }

    io.FontDefault = font;
    io.FontGlobalScale = 1.0f;
    ui_scale_ = scale;
}

void Dx11Renderer::ApplyTheme(const Config::Theme& theme) {
    ImGuiStyle& style = ImGui::GetStyle();
    // Sharp, minimal HUD feel — very small rounding, tight padding
    style.WindowRounding    = 2.0f;
    style.ChildRounding     = 2.0f;
    style.FrameRounding     = 2.0f;
    style.PopupRounding     = 2.0f;
    style.GrabRounding      = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowPadding     = ImVec2(10.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(6.0f, 5.0f);
    style.AntiAliasedLines  = true;
    style.AntiAliasedFill   = true;

    auto color = [](const Config::Color& c) { return ImVec4(c.r, c.g, c.b, c.a); };
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]            = color(theme.text);
    colors[ImGuiCol_TextDisabled]    = color(theme.muted);
    colors[ImGuiCol_WindowBg]        = color(theme.panel);
    colors[ImGuiCol_ChildBg]         = color(theme.panel);
    colors[ImGuiCol_PopupBg]         = color(theme.panel);
    // Border: thin accent-coloured line
    colors[ImGuiCol_Border]          = ImVec4(theme.accent.r, theme.accent.g, theme.accent.b, 0.25f);
    colors[ImGuiCol_FrameBg]         = ImVec4(theme.background.r, theme.background.g, theme.background.b, 0.80f);
    colors[ImGuiCol_FrameBgHovered]  = color(theme.panel_hover);
    colors[ImGuiCol_FrameBgActive]   = color(theme.panel_hover);
    colors[ImGuiCol_TitleBg]         = color(theme.panel);
    colors[ImGuiCol_TitleBgActive]   = color(theme.panel_hover);
    colors[ImGuiCol_CheckMark]       = color(theme.accent);
    colors[ImGuiCol_SliderGrab]      = color(theme.accent);
    colors[ImGuiCol_Button]          = ImVec4(theme.accent.r, theme.accent.g, theme.accent.b, 0.18f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(theme.accent.r, theme.accent.g, theme.accent.b, 0.30f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(theme.accent.r, theme.accent.g, theme.accent.b, 0.44f);
    colors[ImGuiCol_PlotLines]       = color(theme.accent);
    colors[ImGuiCol_PlotHistogram]   = color(theme.accent);  // bars also use accent
}

bool Dx11Renderer::CreateDeviceAndSwapChain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 0;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    constexpr D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    constexpr D3D_DRIVER_TYPE drivers[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
    };

    HRESULT last_hr = E_FAIL;
    for (const D3D_DRIVER_TYPE driver : drivers) {
        D3D_FEATURE_LEVEL selected_level{};
        swap_chain_.Reset();
        device_.Reset();
        context_.Reset();

        const HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            driver,
            nullptr,
            flags,
            feature_levels,
            static_cast<UINT>(std::size(feature_levels)),
            D3D11_SDK_VERSION,
            &desc,
            swap_chain_.GetAddressOf(),
            device_.GetAddressOf(),
            &selected_level,
            context_.GetAddressOf());

        if (SUCCEEDED(hr)) {
            const std::wstring level_text = std::to_wstring(static_cast<unsigned int>(selected_level));
            Log::Info((driver == D3D_DRIVER_TYPE_HARDWARE
                    ? L"DirectX 11 renderer initialized with hardware device. Feature level "
                    : L"DirectX 11 renderer initialized with WARP fallback device. Feature level ") +
                level_text);
            return true;
        }

        last_hr = hr;
        Log::Warn(L"D3D11CreateDeviceAndSwapChain failed with HRESULT " + HResultText(hr));
    }

    Log::Error(L"Failed to create any DirectX 11 device. Last HRESULT " + HResultText(last_hr));
    return false;
}

bool Dx11Renderer::CreateRenderTarget() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())))) {
        return false;
    }
    return SUCCEEDED(device_->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_.GetAddressOf()));
}

void Dx11Renderer::ReleaseRenderTarget() {
    render_target_.Reset();
}

} // namespace overlay
