#pragma once

#include <array>
#include <chrono>
#include <cstdint>

namespace overlay::Telemetry {

constexpr size_t FrametimeHistorySize = 240;

struct MetricSnapshot final {
    std::chrono::steady_clock::time_point sampled_at{};

    float fps{0.0f};
    float fps_1_percent_low{0.0f};
    float fps_0_1_percent_low{0.0f};
    float frametime_ms{0.0f};
    bool has_game_frametime{false};
    std::array<float, FrametimeHistorySize> frametime_history{};
    uint32_t frametime_count{0};

    float cpu_usage_percent{0.0f};
    float cpu_temperature_c{0.0f};
    float cpu_clock_mhz{0.0f};

    float gpu_usage_percent{0.0f};
    float gpu_temperature_c{0.0f};
    float gpu_clock_mhz{0.0f};
    float vram_used_mb{0.0f};
    float vram_total_mb{0.0f};
    float gpu_power_watts{0.0f};

    float ram_used_mb{0.0f};
    float ram_total_mb{0.0f};

    float ping_ms{0.0f};
    float network_rx_kbps{0.0f};
    float network_tx_kbps{0.0f};

    float disk_read_mbps{0.0f};
    float disk_write_mbps{0.0f};
    float disk_active_percent{0.0f};
};

} // namespace overlay::Telemetry

