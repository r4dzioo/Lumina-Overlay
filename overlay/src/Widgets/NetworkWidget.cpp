#include "Overlay/Widgets/StockWidgets.h"

#include <imgui.h>

#include <string>

namespace overlay::Widgets {

namespace {

bool BeginWidget(const char* title, Config::WidgetLayout& layout, bool edit_mode) {
    ImGui::SetNextWindowPos(ImVec2(layout.position.x, layout.position.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(layout.size.x, layout.size.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(layout.opacity);
    const std::string name = std::string(title) + "##" + layout.id;
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing;
    if (!edit_mode) {
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs;
    }
    return ImGui::Begin(name.c_str(), nullptr, flags);
}

void EndWidget(Config::WidgetLayout& layout) {
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    layout.position = {pos.x, pos.y};
    layout.size = {size.x, size.y};
    ImGui::End();
}

} // namespace

void NetworkWidget::Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) {
    if (!BeginWidget(Title(), layout, edit_mode)) {
        EndWidget(layout);
        return;
    }

    ImGui::TextColored(ImVec4(0.05f, 0.82f, 1.0f, 1.0f), "Network");
    ImGui::Text("Ping %.0f ms", snapshot.ping_ms);
    ImGui::TextDisabled("Down %.1f KB/s", snapshot.network_rx_kbps);
    ImGui::TextDisabled("Up   %.1f KB/s", snapshot.network_tx_kbps);

    EndWidget(layout);
}

} // namespace overlay::Widgets
