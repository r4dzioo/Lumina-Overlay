#pragma once

#include "Overlay/Telemetry/MetricSnapshot.h"

#include <array>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace overlay::Telemetry {

class PresentMonClient final {
public:
    bool Initialize(const std::filesystem::path& optional_csv_path);
    void Shutdown();
    void Sample(MetricSnapshot& snapshot);

private:
    bool ReadLatestFrametime(float& frametime_ms);
    static bool TryParseCsvRow(const std::string& line, std::vector<std::string>& values);
    void PushFrametime(float ms, MetricSnapshot& snapshot);
    void ComputeLows(MetricSnapshot& snapshot);

    std::mutex mutex_;
    std::array<float, FrametimeHistorySize> frame_times_{};
    uint32_t write_index_{0};
    uint32_t count_{0};
    bool has_game_data_{false};
    std::filesystem::path csv_path_;
    uintmax_t last_csv_size_{0};
    std::filesystem::file_time_type last_csv_write_time_{};
};

} // namespace overlay::Telemetry
