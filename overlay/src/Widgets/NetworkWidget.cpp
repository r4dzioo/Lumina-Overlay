#include "Overlay/Widgets/StockWidgets.h"

#include <imgui.h>

#include <algorithm>
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

static constexpr ImVec4 kAccent = {0.13f, 0.85f, 0.49f, 1.0f};
static constexpr ImVec4 kLabel  = {0.13f, 0.85f, 0.49f, 0.65f};
static constexpr ImVec4 kHeader = {0.80f, 0.88f, 0.92f, 0.80f};

void NetworkWidget::Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) {
    if (!BeginWidget(Title(), layout, edit_mode)) {
        EndWidget(layout);
        return;
    }

    const float win_w = ImGui::GetContentRegionAvail().x;

    ImGui::TextColored(kHeader, "NETWORK");
    {
        char buf[20];
        snprintf(buf, sizeof(buf), "%.0f ms", snapshot.ping_ms);
        ImGui::SameLine(win_w - ImGui::CalcTextSize(buf).x);
        ImGui::TextColored(snapshot.ping_ms > 80.0f ? ImVec4(1.0f, 0.75f, 0.25f, 1.0f) : kAccent, "%s", buf);
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.08f, 0.10f, 0.60f));
    ImGui::ProgressBar(std::min(snapshot.ping_ms / 200.0f, 1.0f), ImVec2(-1.0f, 4.0f), "");
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::TextColored(kLabel, "DN"); ImGui::SameLine(0, 4);
    if (snapshot.network_rx_kbps >= 1024.0f)
        ImGui::Text("%.1f MB/s", snapshot.network_rx_kbps / 1024.0f);
    else
        ImGui::Text("%.0f KB/s", snapshot.network_rx_kbps);
    ImGui::SameLine(win_w * 0.50f);
    ImGui::TextColored(kLabel, "UP"); ImGui::SameLine(0, 4);
    if (snapshot.network_tx_kbps >= 1024.0f)
        ImGui::Text("%.1f MB/s", snapshot.network_tx_kbps / 1024.0f);
    else
        ImGui::Text("%.0f KB/s", snapshot.network_tx_kbps);

    EndWidget(layout);
}

} // namespace overlay::Widgets
