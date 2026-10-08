#pragma once

#include "Overlay/Config/OverlayConfig.h"
#include "Overlay/Telemetry/MetricSnapshot.h"

#include <string>

namespace overlay::Widgets {

class IWidget {
public:
    virtual ~IWidget() = default;
    virtual const char* Id() const = 0;
    virtual const char* Title() const = 0;
    virtual void Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) = 0;
};

} // namespace overlay::Widgets
