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

    ImGui::SetNextWindowSize(ImVec2(520.0f, 560.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(60.0f, 60.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Lumina Overlay — Settings", &settings_visible_,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    // Emerald accent
    const ImVec4 accent  = ImVec4(0.13f, 0.85f, 0.49f, 1.0f);
    const ImVec4 warning = ImVec4(0.96f, 0.65f, 0.14f, 1.0f);
    const ImVec4 muted   = ImVec4(0.48f, 0.60f, 0.70f, 1.0f);

    // ── Live metrics ─────────────────────────────────────────────────────
    ImGui::TextColored(accent, "LIVE METRICS");
    ImGui::Separator();
    if (snapshot.has_game_frametime) {
        ImGui::Text("FPS  "); ImGui::SameLine(0, 0);
        ImGui::TextColored(accent, "%.0f", snapshot.fps);
        ImGui::SameLine(110.0f);
        ImGui::Text("Frametime  "); ImGui::SameLine(0, 0);
        ImGui::TextColored(accent, "%.2f ms", snapshot.frametime_ms);
        if (snapshot.fps_1_percent_low > 0.0f) {
            ImGui::Text("1%% Low  "); ImGui::SameLine(0, 0);
            ImGui::TextColored(accent, "%.0f", snapshot.fps_1_percent_low);
            ImGui::SameLine(110.0f);
            ImGui::Text("0.1%% Low  "); ImGui::SameLine(0, 0);
            ImGui::TextColored(accent, "%.0f", snapshot.fps_0_1_percent_low);
        }
    } else {
        ImGui::TextColored(muted, "FPS — waiting for frames...");
    }
    ImGui::Text("CPU  "); ImGui::SameLine(0, 0);
    ImGui::TextColored(
        snapshot.cpu_usage_percent > 90.0f ? warning : accent,
        "%.0f%%", snapshot.cpu_usage_percent);
    ImGui::SameLine(110.0f);
    ImGui::Text("GPU  "); ImGui::SameLine(0, 0);
    ImGui::TextColored(
        snapshot.gpu_usage_percent > 90.0f ? warning : accent,
        "%.0f%%", snapshot.gpu_usage_percent);
    ImGui::Text("RAM  "); ImGui::SameLine(0, 0);
    ImGui::TextColored(accent, "%.0f / %.0f MB", snapshot.ram_used_mb, snapshot.ram_total_mb);
    ImGui::Spacing();

    // ── Display ──────────────────────────────────────────────────────────
    ImGui::TextColored(accent, "DISPLAY");
    ImGui::Separator();

    float scale = profile.global_scale;
    if (ImGui::SliderFloat("UI Scale", &scale, 0.75f, 1.75f, "%.2f")) {
        config_.SetGlobalScale(scale);
    }

    // Per-profile global opacity
    float global_opacity = profile.opacity;
    if (ImGui::SliderFloat("Global Opacity", &global_opacity, 0.20f, 1.00f, "%.2f")) {
        for (auto& p : config_.Snapshot().profiles) {
            if (p.name == profile.name) {
                // direct setter via ConfigManager
            }
        }
        config_.SetGlobalOpacity(global_opacity);
    }

    bool click_through = profile.click_through;
    if (ImGui::Checkbox("Click-through (non-edit mode)", &click_through)) {
        config_.SetClickThrough(click_through);
    }

    bool perf_mode = performance_mode_;
    if (ImGui::Checkbox("Performance mode  (unlock framerate)", &perf_mode)) {
        performance_mode_ = perf_mode;
    }

    bool rgb = profile.rgb_accent;
    if (ImGui::Checkbox("RGB accent cycling", &rgb)) {
        config_.SetRgbAccent(rgb);
    }
    ImGui::Spacing();

    // ── Widgets ───────────────────────────────────────────────────────────
    ImGui::TextColored(accent, "WIDGETS");
    ImGui::Separator();
    ImGui::TextColored(muted, "Toggle  /  drag to reposition in edit mode (F10)");
    ImGui::Spacing();

    for (auto& layout : profile.widgets) {
        bool enabled = layout.enabled;
        if (ImGui::Checkbox(layout.id.c_str(), &enabled)) {
            layout.enabled = enabled;
            config_.UpdateWidgetLayout(layout);
        }
        // Per-widget opacity slider on the same line
        ImGui::SameLine(130.0f);
        ImGui::PushID(layout.id.c_str());
        float op = layout.opacity;
        ImGui::SetNextItemWidth(180.0f);
        if (ImGui::SliderFloat("opacity", &op, 0.10f, 1.00f, "%.2f")) {
            layout.opacity = op;
            config_.UpdateWidgetLayout(layout);
        }
        ImGui::PopID();
    }
    ImGui::Spacing();

    // ── Actions ───────────────────────────────────────────────────────────
    ImGui::TextColored(accent, "ACTIONS");
    ImGui::Separator();
    if (ImGui::Button("Save layout & settings")) {
        config_.Save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset widget positions")) {
        config_.ResetWidgetPositions();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close  [F10]")) {
        settings_visible_ = false;
    }

    ImGui::Spacing();
    ImGui::TextColored(muted, "Insert = toggle overlay   F10 = settings   F11 = perf mode");
    ImGui::End();
}

} // namespace overlay
