#include "Overlay/App.h"

#include "Overlay/CrashHandler.h"
#include "Overlay/Log.h"

#include <imgui.h>
#include <mmsystem.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

namespace overlay {

namespace {

std::filesystem::path ModuleDirectory() {
    std::wstring buffer(32768, L'\0');
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    buffer.resize(size);
    return std::filesystem::path(buffer).parent_path();
}

} // namespace

App::App() = default;

App::~App() {
    config_.Save();
    telemetry_.Stop();
    renderer_.Shutdown();
    window_.Destroy();
    timeEndPeriod(1);
}

int App::Run(HINSTANCE instance, int show_command) {
    if (!Initialize(instance, show_command)) {
        return EXIT_FAILURE;
    }

    MainLoop();
    return EXIT_SUCCESS;
}

void App::RequestQuit() {
    running_ = false;
}

bool App::Initialize(HINSTANCE instance, int show_command) {
    timeBeginPeriod(1);

    const auto app_dir = ModuleDirectory();
    Log::Initialize(app_dir / "logs" / "overlay.log");
    CrashHandler::Install();

    if (!config_.Initialize(app_dir)) {
        Log::Warn(L"Failed to load config; defaults were applied.");
    }

    widgets_.RegisterDefaults();
    plugin_loader_.LoadFromDirectory(app_dir / "plugins", widgets_);

    const auto theme = config_.ActiveTheme();
    if (!window_.Create(instance, show_command, [this](UINT width, UINT height) {
            renderer_.Resize(width, height);
        })) {
        Log::Error(L"Failed to create overlay window.");
        return false;
    }

    if (!renderer_.Initialize(window_.Handle(), theme)) {
        Log::Error(L"Failed to initialize DirectX 11 renderer.");
        return false;
    }

    const auto profile = config_.ActiveProfile();
    window_.SetClickThrough(profile.click_through);
    window_.SetOpacity(profile.opacity);

    telemetry_.Start(app_dir);
    running_ = true;
    return true;
}

void App::MainLoop() {
    using namespace std::chrono_literals;

    while (running_) {
        if (!window_.PumpMessages()) {
            running_ = false;
            break;
        }

        ApplyHotkeys();
        window_.SetTopMost();

        // Count this frame before rendering so the telemetry service can
        // compute overlay-side FPS even without an external PresentMon CSV.
        telemetry_.RecordRenderFrame();

        RenderFrame();
        std::this_thread::sleep_for(performance_mode_ ? 1ms : 4ms);
    }
}

void App::ApplyHotkeys() {
    hotkeys_.Update();

    if (hotkeys_.ConsumeToggleOverlay()) {
        overlay_visible_ = !overlay_visible_;
    }

    if (hotkeys_.ConsumeToggleSettings()) {
        settings_visible_ = !settings_visible_;
        overlay_visible_ = true;
    }

    if (hotkeys_.ConsumePerformanceMode()) {
        performance_mode_ = !performance_mode_;
    }

    const auto profile = config_.ActiveProfile();
    window_.SetClickThrough(!settings_visible_ && profile.click_through);
    window_.SetOpacity(profile.opacity);
}

void App::RenderFrame() {
    renderer_.BeginFrame();

    if (overlay_visible_) {
        auto profile = config_.ActiveProfile();
        const auto snapshot = telemetry_.Snapshot();

        renderer_.SetUiScale(profile.global_scale);
        widgets_.RenderAll(snapshot, profile, settings_visible_);
        for (const auto& layout : profile.widgets) {
            config_.UpdateWidgetLayout(layout);
        }

        if (settings_visible_) {
            RenderSettingsWindow(snapshot);
        }
    }

    renderer_.EndFrame(performance_mode_);
}

void App::RenderSettingsWindow(const Telemetry::MetricSnapshot& snapshot) {
    auto profile = config_.ActiveProfile();

    ImGui::SetNextWindowSize(ImVec2(460.0f, 420.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Lumina Overlay Settings", &settings_visible_, ImGuiWindowFlags_NoCollapse);

    // Accent colour matching the new yellow-green theme
    const ImVec4 accent = ImVec4(0.68f, 1.0f, 0.0f, 1.0f);

    ImGui::TextColored(accent, "Runtime");
    if (snapshot.has_game_frametime) {
        ImGui::Text("FPS %.0f   Frametime %.2f ms", snapshot.fps, snapshot.frametime_ms);
    } else {
        ImGui::TextDisabled("FPS — initializing...");
    }
    ImGui::TextDisabled("F10 edit mode unlocks widgets. Close settings to make overlay click-through.");
    ImGui::Checkbox("Performance mode", &performance_mode_);

    bool click_through = profile.click_through;
    if (ImGui::Checkbox("Click-through overlay", &click_through)) {
        config_.SetClickThrough(click_through);
    }

    float scale = profile.global_scale;
    if (ImGui::SliderFloat("Scale", &scale, 0.75f, 1.75f, "%.2f")) {
        config_.SetGlobalScale(scale);
    }

    ImGui::Separator();
    ImGui::TextColored(accent, "Widgets");
    for (auto& layout : profile.widgets) {
        bool enabled = layout.enabled;
        if (ImGui::Checkbox(layout.id.c_str(), &enabled)) {
            layout.enabled = enabled;
            config_.UpdateWidgetLayout(layout);
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Save layout")) {
        config_.Save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        settings_visible_ = false;
    }

    ImGui::TextDisabled("Insert toggles overlay. F10 opens settings. F11 toggles performance mode.");
    ImGui::End();
}

} // namespace overlay
