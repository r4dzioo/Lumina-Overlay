#pragma once

#include "Overlay/Telemetry/MetricSnapshot.h"

namespace overlay::Telemetry {

class GpuProvider final {
public:
    bool Initialize();
    void Shutdown();
    void Sample(MetricSnapshot& snapshot);

private:
    bool initialized_{false};
};

} // namespace overlay::Telemetry

