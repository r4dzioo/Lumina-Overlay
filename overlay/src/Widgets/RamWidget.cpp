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

static constexpr ImVec4 kAccent = {0.13f, 0.85f, 0.49f, 1.0f};
static constexpr ImVec4 kLabel  = {0.13f, 0.85f, 0.49f, 0.65f};
static constexpr ImVec4 kHeader = {0.80f, 0.88f, 0.92f, 0.80f};

} // namespace

void RamWidget::Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) {
    if (!BeginWidget(Title(), layout, edit_mode)) {
        EndWidget(layout);
        return;
    }

    const float ratio  = snapshot.ram_total_mb > 0.0f
                         ? snapshot.ram_used_mb / snapshot.ram_total_mb : 0.0f;
    const float win_w  = ImGui::GetContentRegionAvail().x;
    const float used_g = snapshot.ram_used_mb  / 1024.0f;
    const float tot_g  = snapshot.ram_total_mb / 1024.0f;

    ImGui::TextColored(kHeader, "RAM");
    {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f / %.1f G", used_g, tot_g);
        ImGui::SameLine(win_w - ImGui::CalcTextSize(buf).x);
        ImGui::TextColored(kAccent, "%s", buf);
    }

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.10f, 0.12f, 0.08f, 0.60f));
    ImGui::ProgressBar(ratio, ImVec2(-1.0f, 5.0f), "");
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::TextColored(kLabel, "USAGE"); ImGui::SameLine(0, 4);
    ImGui::Text("%.0f%%", ratio * 100.0f);

    EndWidget(layout);
}

} // namespace overlay::Widgets
