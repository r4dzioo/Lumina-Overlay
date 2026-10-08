#pragma once

#include "Overlay/Telemetry/MetricSnapshot.h"

#include <pdh.h>

#include <vector>

namespace overlay::Telemetry {

class PdhSampler final {
public:
    PdhSampler() = default;
    ~PdhSampler();

    PdhSampler(const PdhSampler&) = delete;
    PdhSampler& operator=(const PdhSampler&) = delete;

    bool Initialize();
    void Shutdown();
    void Sample(MetricSnapshot& snapshot);

private:
    bool AddCounter(const wchar_t* path, PDH_HCOUNTER& counter);
    static float ReadCounter(PDH_HCOUNTER counter);

    PDH_HQUERY query_{nullptr};
    PDH_HCOUNTER cpu_total_{nullptr};
    PDH_HCOUNTER cpu_clock_{nullptr};
    PDH_HCOUNTER disk_read_{nullptr};
    PDH_HCOUNTER disk_write_{nullptr};
    PDH_HCOUNTER disk_active_{nullptr};
    PDH_HCOUNTER network_rx_{nullptr};
    PDH_HCOUNTER network_tx_{nullptr};
};

} // namespace overlay::Telemetry
