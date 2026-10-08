#pragma once

#include "Overlay/Config/ConfigManager.h"
#include "Overlay/Dx11Renderer.h"
#include "Overlay/Hotkeys.h"
#include "Overlay/Telemetry/TelemetryService.h"
#include "Overlay/Widgets/PluginLoader.h"
#include "Overlay/Widgets/WidgetRegistry.h"
#include "Overlay/Win32Window.h"

#include <atomic>
#include <memory>

namespace overlay {

class App final {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int Run(HINSTANCE instance, int show_command);
    void RequestQuit();

private:
    bool Initialize(HINSTANCE instance, int show_command);
    void MainLoop();
    void RenderFrame();
    void ApplyHotkeys();
    void RenderSettingsWindow(const Telemetry::MetricSnapshot& snapshot);

    Config::ConfigManager config_;
    Win32Window window_;
    Dx11Renderer renderer_;
    Telemetry::TelemetryService telemetry_;
    Widgets::WidgetRegistry widgets_;
    Widgets::PluginLoader plugin_loader_;
    Hotkeys hotkeys_;

    std::atomic_bool running_{false};
    bool overlay_visible_{true};
    bool settings_visible_{false};
    bool performance_mode_{false};
};

} // namespace overlay
