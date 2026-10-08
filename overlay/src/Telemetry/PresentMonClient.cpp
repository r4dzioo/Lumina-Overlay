#include "Overlay/Telemetry/PresentMonClient.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <fstream>
#include <numeric>
#include <sstream>
#include <vector>

namespace overlay::Telemetry {

bool PresentMonClient::Initialize(const std::filesystem::path& optional_csv_path) {
    csv_path_ = optional_csv_path;
    return true;
}

void PresentMonClient::Shutdown() {
    std::lock_guard lock(mutex_);
    count_ = 0;
    write_index_ = 0;
    has_game_data_ = false;
    frame_times_.fill(0.0f);
    last_csv_size_ = 0;
    last_csv_write_time_ = {};
}

void PresentMonClient::Sample(MetricSnapshot& snapshot) {
    std::lock_guard lock(mutex_);

    float frametime_ms = 0.0f;
    const bool received_game_sample = ReadLatestFrametime(frametime_ms);

    if (received_game_sample) {
        PushFrametime(frametime_ms, snapshot);
        snapshot.has_game_frametime = true;
    } else {
        snapshot.frametime_count = count_;
        snapshot.frametime_history = frame_times_;
        if (count_ > 0) {
            const uint32_t last_index = write_index_ == 0 ? static_cast<uint32_t>(FrametimeHistorySize - 1) : write_index_ - 1;
            snapshot.frametime_ms = frame_times_[last_index];
            snapshot.fps = snapshot.frametime_ms > 0.0f ? 1000.0f / snapshot.frametime_ms : 0.0f;
        }
        snapshot.has_game_frametime = has_game_data_;
    }

    ComputeLows(snapshot);
}

bool PresentMonClient::ReadLatestFrametime(float& frametime_ms) {
    if (csv_path_.empty() || !std::filesystem::exists(csv_path_)) {
        return false;
    }

    const uintmax_t csv_size = std::filesystem::file_size(csv_path_);
    const auto write_time = std::filesystem::last_write_time(csv_path_);
    if (csv_size == last_csv_size_ && write_time == last_csv_write_time_) {
        return false;
    }
    last_csv_size_ = csv_size;
    last_csv_write_time_ = write_time;

    std::ifstream stream(csv_path_);
    std::string header;
    std::string line;
    std::string last_line;
    if (!std::getline(stream, header)) {
        return false;
    }
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            last_line = line;
        }
    }
    if (last_line.empty()) {
        return false;
    }

    std::vector<std::string> columns;
    std::vector<std::string> values;
    if (!TryParseCsvRow(header, columns) || !TryParseCsvRow(last_line, values)) {
        return false;
    }
    for (size_t index = 0; index < columns.size() && index < values.size(); ++index) {
        if (columns[index] != "MsBetweenPresents" && columns[index] != "MsBetweenDisplayChange") {
            continue;
        }
        try {
            const float value = std::stof(values[index]);
            if (value > 0.2f && value < 1000.0f) {
                frametime_ms = value;
                return true;
            }
        } catch (...) {
            return false;
        }
    }
    return false;
}

bool PresentMonClient::TryParseCsvRow(const std::string& line, std::vector<std::string>& values) {
    values.clear();
    std::string value;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char character = line[i];
        if (character == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
                value += character;
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (character == ',' && !quoted) {
            values.push_back(value);
            value.clear();
        } else if (character != '\r') {
            value += character;
        }
    }
    values.push_back(value);
    return !quoted;
}

void PresentMonClient::PushFrametime(float ms, MetricSnapshot& snapshot) {
    frame_times_[write_index_ % FrametimeHistorySize] = ms;
    write_index_ = (write_index_ + 1) % FrametimeHistorySize;
    count_ = std::min<uint32_t>(count_ + 1, static_cast<uint32_t>(FrametimeHistorySize));

    snapshot.frametime_ms = ms;
    snapshot.fps = ms > 0.0f ? 1000.0f / ms : 0.0f;
    snapshot.has_game_frametime = true;
    has_game_data_ = true;
    snapshot.frametime_count = count_;
    snapshot.frametime_history = frame_times_;
}

void PresentMonClient::ComputeLows(MetricSnapshot& snapshot) {
    if (count_ == 0) {
        return;
    }

    std::array<float, FrametimeHistorySize> sorted{};
    size_t valid_count = 0;
    for (uint32_t i = 0; i < count_; ++i) {
        const float value = frame_times_[i];
        if (value > 0.0f) {
            sorted[valid_count++] = value;
        }
    }

    if (valid_count == 0) {
        return;
    }

    std::sort(sorted.begin(), sorted.begin() + valid_count, std::greater<float>());
    const auto average_slowest = [&](float percent) {
        const size_t sample_count = std::max<size_t>(1, static_cast<size_t>(valid_count * percent));
        const float total = std::accumulate(sorted.begin(), sorted.begin() + sample_count, 0.0f);
        return total / static_cast<float>(sample_count);
    };

    const float one_percent_ms = average_slowest(0.01f);
    const float point_one_percent_ms = average_slowest(0.001f);
    snapshot.fps_1_percent_low = one_percent_ms > 0.0f ? 1000.0f / one_percent_ms : 0.0f;
    snapshot.fps_0_1_percent_low = point_one_percent_ms > 0.0f ? 1000.0f / point_one_percent_ms : 0.0f;
}

} // namespace overlay::Telemetry
