#pragma once

#include "Overlay/Telemetry/GpuProvider.h"
#include "Overlay/Telemetry/MetricSnapshot.h"
#include "Overlay/Telemetry/PdhSampler.h"
#include "Overlay/Telemetry/PresentMonClient.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <thread>

namespace overlay::Telemetry {

class TelemetryService final {
public:
    TelemetryService() = default;
    ~TelemetryService();

    TelemetryService(const TelemetryService&) = delete;
    TelemetryService& operator=(const TelemetryService&) = delete;

    bool Start(const std::filesystem::path& app_directory);
    void Stop();

    MetricSnapshot Snapshot() const;

    // Called from the render loop each frame so we can compute overlay FPS
    void RecordRenderFrame();

private:
    void ThreadMain();
    void Publish(const MetricSnapshot& snapshot);
    void SampleMemory(MetricSnapshot& snapshot);
    void SamplePing(MetricSnapshot& snapshot);
    void SampleOverlayFps(MetricSnapshot& snapshot);

    mutable std::mutex snapshot_mutex_;
    MetricSnapshot snapshot_{};

    PdhSampler pdh_;
    GpuProvider gpu_;
    PresentMonClient present_mon_;

    std::thread worker_;
    std::atomic_bool running_{false};
    std::filesystem::path app_directory_;

    // Overlay-side FPS counter (always available, no external tool needed)
    mutable std::mutex fps_mutex_;
    uint32_t frame_count_{0};
    std::chrono::steady_clock::time_point fps_window_start_{};
    float overlay_fps_{0.0f};
};

} // namespace overlay::Telemetry
