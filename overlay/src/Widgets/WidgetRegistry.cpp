#include "Overlay/Widgets/WidgetRegistry.h"

#include "Overlay/Widgets/StockWidgets.h"

namespace overlay::Widgets {

void WidgetRegistry::RegisterDefaults() {
    Register(std::make_unique<FpsWidget>());
    Register(std::make_unique<FrametimeWidget>());
    Register(std::make_unique<CpuWidget>());
    Register(std::make_unique<GpuWidget>());
    Register(std::make_unique<RamWidget>());
    Register(std::make_unique<NetworkWidget>());
}

void WidgetRegistry::Register(std::unique_ptr<IWidget> widget) {
    IWidget* raw = widget.get();
    by_id_[raw->Id()] = raw;
    widgets_.push_back(std::move(widget));
}

void WidgetRegistry::RenderAll(const Telemetry::MetricSnapshot& snapshot, Config::Profile& profile, bool edit_mode) {
    for (auto& layout : profile.widgets) {
        if (!layout.enabled) {
            continue;
        }

        IWidget* widget = Find(layout.id);
        if (widget) {
            widget->Render(snapshot, layout, edit_mode);
        }
    }
}

IWidget* WidgetRegistry::Find(const std::string& id) {
    const auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : it->second;
}

} // namespace overlay::Widgets
