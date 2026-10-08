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
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoTitleBar;
    if (!edit_mode) {
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs;
    }
    return ImGui::Begin(name.c_str(), nullptr, flags);
}

void EndWidget(Config::WidgetLayout& layout) {
    const ImVec2 pos  = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    layout.position = {pos.x, pos.y};
    layout.size     = {size.x, size.y};
    ImGui::End();
}

static constexpr ImVec4 kAccent = {0.68f, 1.00f, 0.00f, 1.0f};
static constexpr ImVec4 kLabel  = {0.68f, 1.00f, 0.00f, 0.65f};
static constexpr ImVec4 kHeader = {0.80f, 0.85f, 0.80f, 0.80f};

} // namespace

void CpuWidget::Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) {
    if (!BeginWidget(Title(), layout, edit_mode)) {
        EndWidget(layout);
        return;
    }

    const float win_w = ImGui::GetContentRegionAvail().x;

    // Header
    ImGui::TextColored(kHeader, "CPU");
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f%%", snapshot.cpu_usage_percent);
        ImGui::SameLine(win_w - ImGui::CalcTextSize(buf).x);
        ImGui::TextColored(kAccent, "%s", buf);
    }

    // Progress bar
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.10f, 0.12f, 0.08f, 0.60f));
    ImGui::ProgressBar(snapshot.cpu_usage_percent / 100.0f, ImVec2(-1.0f, 5.0f), "");
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::TextColored(kLabel, "TEMP"); ImGui::SameLine(0, 4);
    ImGui::Text("%.0f C", snapshot.cpu_temperature_c);
    ImGui::SameLine(win_w * 0.50f);
    ImGui::TextColored(kLabel, "CLK"); ImGui::SameLine(0, 4);
    ImGui::Text("%.0f MHz", snapshot.cpu_clock_mhz);

    ImGui::TextColored(kLabel, "DISK R"); ImGui::SameLine(0, 4);
    ImGui::Text("%.1f MB/s", snapshot.disk_read_mbps);
    ImGui::SameLine(win_w * 0.50f);
    ImGui::TextColored(kLabel, "W"); ImGui::SameLine(0, 4);
    ImGui::Text("%.1f MB/s", snapshot.disk_write_mbps);

    EndWidget(layout);
}

} // namespace overlay::Widgets
