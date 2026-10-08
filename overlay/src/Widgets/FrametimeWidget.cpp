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

void FrametimeWidget::Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) {
    if (!BeginWidget(Title(), layout, edit_mode)) {
        EndWidget(layout);
        return;
    }

    const float win_w = ImGui::GetContentRegionAvail().x;

    // Header row
    ImGui::TextColored(kHeader, "FRAME TIME");
    if (!snapshot.has_game_frametime) {
        ImGui::SameLine(win_w - ImGui::CalcTextSize("---").x);
        ImGui::TextDisabled("---");
    } else {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f ms", snapshot.frametime_ms);
        ImGui::SameLine(win_w - ImGui::CalcTextSize(buf).x);
        ImGui::TextColored(kAccent, "%s", buf);
    }

    // Separator
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p.x, p.y), ImVec2(p.x + win_w, p.y),
            IM_COL32(100, 140, 60, 70), 1.0f);
        ImGui::Dummy(ImVec2(0, 4));
    }

    if (!snapshot.has_game_frametime) {
        ImGui::TextDisabled("Waiting for frames...");
        EndWidget(layout);
        return;
    }

    const float max_ms = std::max(20.0f, snapshot.frametime_ms * 1.8f);
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,   ImVec4(0.0f, 0.0f, 0.0f, 0.30f));
    ImGui::PlotLines(
        "##frametime-plot",
        snapshot.frametime_history.data(),
        static_cast<int>(Telemetry::FrametimeHistorySize),
        0,
        nullptr,
        0.0f,
        max_ms,
        ImVec2(-1.0f, ImGui::GetFontSize() * 4.2f));
    ImGui::PopStyleColor(2);

    ImGui::TextColored(kLabel, "Lower and flatter is better");
    EndWidget(layout);
}

} // namespace overlay::Widgets
