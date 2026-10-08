#include "Overlay/Telemetry/TelemetryService.h"

#include "Overlay/Log.h"

#include <windows.h>

#include <chrono>

namespace overlay::Telemetry {

TelemetryService::~TelemetryService() {
    Stop();
}

bool TelemetryService::Start(const std::filesystem::path& app_directory) {
    if (running_.exchange(true)) {
        return true;
    }

    app_directory_ = app_directory;
    pdh_.Initialize();
    gpu_.Initialize();
    present_mon_.Initialize(app_directory_ / "assets" / "presentmon.csv");

    fps_window_start_ = std::chrono::steady_clock::now();
    frame_count_ = 0;

    worker_ = std::thread(&TelemetryService::ThreadMain, this);
    Log::Info(L"Telemetry service started.");
    return true;
}

void TelemetryService::Stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    present_mon_.Shutdown();
    gpu_.Shutdown();
    pdh_.Shutdown();
    Log::Info(L"Telemetry service stopped.");
}

MetricSnapshot TelemetryService::Snapshot() const {
    std::lock_guard lock(snapshot_mutex_);
    return snapshot_;
}

void TelemetryService::RecordRenderFrame() {
    std::lock_guard lock(fps_mutex_);
    ++frame_count_;
}

void TelemetryService::ThreadMain() {
    using namespace std::chrono_literals;

    while (running_) {
        MetricSnapshot snapshot{};
        snapshot.sampled_at = std::chrono::steady_clock::now();

        present_mon_.Sample(snapshot);
        SampleOverlayFps(snapshot);
        pdh_.Sample(snapshot);
        SampleMemory(snapshot);
        SamplePing(snapshot);
        gpu_.Sample(snapshot);

        Publish(snapshot);
        std::this_thread::sleep_for(250ms);
    }
}

void TelemetryService::SampleOverlayFps(MetricSnapshot& snapshot) {
    // If PresentMon already has game data, let it win — it's more accurate.
    if (snapshot.has_game_frametime) {
        return;
    }

    std::lock_guard lock(fps_mutex_);
    using namespace std::chrono;

    const auto now = steady_clock::now();
    const float elapsed_sec =
        duration_cast<duration<float>>(now - fps_window_start_).count();

    // Refresh the FPS estimate every ~500 ms
    if (elapsed_sec >= 0.5f && frame_count_ > 0) {
        overlay_fps_ = static_cast<float>(frame_count_) / elapsed_sec;
        frame_count_ = 0;
        fps_window_start_ = now;
    }

    if (overlay_fps_ > 0.0f) {
        const float frametime_ms = 1000.0f / overlay_fps_;

        // Push the computed frametime into the snapshot so the graph works too
        snapshot.fps = overlay_fps_;
        snapshot.frametime_ms = frametime_ms;
        snapshot.has_game_frametime = true;  // mark as valid so widgets render

        // Populate frametime_history with the current value (flat line while
        // we only have one data point — it fills properly during PresentMon use)
        snapshot.frametime_count = 1;
        snapshot.frametime_history.fill(frametime_ms);
    }
}

void TelemetryService::Publish(const MetricSnapshot& snapshot) {
    std::lock_guard lock(snapshot_mutex_);
    snapshot_ = snapshot;
}

void TelemetryService::SampleMemory(MetricSnapshot& snapshot) {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        snapshot.ram_total_mb = static_cast<float>(status.ullTotalPhys / (1024ull * 1024ull));
        snapshot.ram_used_mb  = static_cast<float>((status.ullTotalPhys - status.ullAvailPhys) / (1024ull * 1024ull));
    }
}

void TelemetryService::SamplePing(MetricSnapshot& snapshot) {
    snapshot.ping_ms = 0.0f;
}

} // namespace overlay::Telemetry
