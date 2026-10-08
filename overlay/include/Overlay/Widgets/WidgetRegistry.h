#pragma once

#include "Overlay/Config/OverlayConfig.h"
#include "Overlay/Telemetry/MetricSnapshot.h"
#include "Overlay/Widgets/IWidget.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace overlay::Widgets {

class WidgetRegistry final {
public:
    void RegisterDefaults();
    void Register(std::unique_ptr<IWidget> widget);
    void RenderAll(const Telemetry::MetricSnapshot& snapshot, Config::Profile& profile, bool edit_mode);

private:
    IWidget* Find(const std::string& id);

    std::vector<std::unique_ptr<IWidget>> widgets_;
    std::unordered_map<std::string, IWidget*> by_id_;
};

} // namespace overlay::Widgets
