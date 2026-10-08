#include "Overlay/Widgets/StockWidgets.h"

#include <imgui.h>

#include <cmath>
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

// Yellow-green accent: #ADFF00
static constexpr ImVec4 kAccent   = {0.68f, 1.00f, 0.00f, 1.0f};
// Dimmer label colour
static constexpr ImVec4 kLabel    = {0.68f, 1.00f, 0.00f, 0.72f};
// Section header (small caps feel)
static constexpr ImVec4 kHeader   = {0.80f, 0.85f, 0.90f, 0.80f};
// Right-aligned status tag colour
static constexpr ImVec4 kStatus   = {0.68f, 1.00f, 0.00f, 0.90f};

// Determine a simple stability label based on 1% lows vs avg FPS
const char* StabilityLabel(float fps, float fps_1_low) {
    if (fps <= 0.0f)   return "---";
    if (fps_1_low <= 0.0f) return "LIVE";
    const float ratio = fps_1_low / fps;
    if (ratio >= 0.85f) return "STABLE";
    if (ratio >= 0.65f) return "VARIABLE";
    return "UNSTABLE";
}

} // namespace

void FpsWidget::Render(const Telemetry::MetricSnapshot& snapshot, Config::WidgetLayout& layout, bool edit_mode) {
    if (!BeginWidget(Title(), layout, edit_mode)) {
        EndWidget(layout);
        return;
    }

    const float win_w = ImGui::GetContentRegionAvail().x;

    // ── Row 1: "PERFORMANCE" title  ·  "F11 / LIVE" tag ──────────────────
    ImGui::TextColored(kHeader, "PERFORMANCE");
    {
        const char* tag = "F11 / LIVE";
        const float tag_w = ImGui::CalcTextSize(tag).x;
        ImGui::SameLine(win_w - tag_w);
        ImGui::TextColored(kStatus, "%s", tag);
    }

    ImGui::Spacing();
    // Thin separator line via the draw list
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p.x, p.y),
            ImVec2(p.x + win_w, p.y),
            IM_COL32(100, 120, 100, 80), 1.0f);
        ImGui::Dummy(ImVec2(0, 4));
    }

    if (!snapshot.has_game_frametime) {
        ImGui::TextDisabled("FPS");
        ImGui::Spacing();
        ImGui::TextDisabled("Initializing...");
        ImGui::TextDisabled("Waiting for render frames");
        EndWidget(layout);
        return;
    }

    // ── Row 2: small labels ───────────────────────────────────────────────
    {
        const char* ft_label = "FRAME TIME";
        const float ft_w = ImGui::CalcTextSize(ft_label).x;
        ImGui::TextColored(kLabel, "FPS");
        ImGui::SameLine(win_w / 2.0f);
        ImGui::TextColored(kLabel, "%s", ft_label);
        (void)ft_w;
    }

    // ── Row 3: big numbers ────────────────────────────────────────────────
    {
        // Scale the big font by temporarily nudging FontGlobalScale
        // (Dear ImGui doesn't support per-text font sizes without a font atlas,
        //  so we use a size multiplier trick with Dummy padding instead)

        const char* fps_str = "---";
        char fps_buf[16]  = {};
        char ft_buf[16]   = {};

        if (snapshot.fps > 0.0f) {
            snprintf(fps_buf, sizeof(fps_buf), "%.0f", snapshot.fps);
            fps_str = fps_buf;
        }
        if (snapshot.frametime_ms > 0.0f) {
            snprintf(ft_buf, sizeof(ft_buf), "%.1f", snapshot.frametime_ms);
        } else {
            snprintf(ft_buf, sizeof(ft_buf), "--.-");
        }

        // Push a 2× scaled font via FontGlobalScale
        ImGuiIO& io = ImGui::GetIO();
        const float prev_scale = io.FontGlobalScale;
        io.FontGlobalScale = prev_scale * 2.0f;

        ImGui::TextColored(kAccent, "%s", fps_str);
        ImGui::SameLine(win_w / 2.0f);
        ImGui::TextColored(kAccent, "%s", ft_buf);

        io.FontGlobalScale = prev_scale;
    }

    // ── Row 4: stability + "MS / AVG" ─────────────────────────────────────
    {
        const char* stability = StabilityLabel(snapshot.fps, snapshot.fps_1_percent_low);
        ImGui::TextColored(kAccent, "%s", stability);
        ImGui::SameLine(win_w / 2.0f);
        ImGui::TextColored(kLabel, "MS / AVG");
    }

    ImGui::Spacing();

    // ── Row 5: FPS graph (line plot) ──────────────────────────────────────
    {
        // Build fps_from_frametime history for the plot
        constexpr int kHistSize = static_cast<int>(Telemetry::FrametimeHistorySize);
        static float fps_plot[kHistSize];
        for (int i = 0; i < kHistSize; ++i) {
            const float ft = snapshot.frametime_history[i];
            fps_plot[i] = (ft > 0.0f) ? 1000.0f / ft : 0.0f;
        }

        const float plot_h = ImGui::GetFontSize() * 3.8f;
        ImGui::PushStyleColor(ImGuiCol_PlotLines,    ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg,      ImVec4(0.0f, 0.0f, 0.0f, 0.35f));
        ImGui::PlotLines(
            "##fps-graph",
            fps_plot,
            kHistSize,
            0,
            nullptr,
            0.0f, 0.0f,           // auto scale
            ImVec2(-1.0f, plot_h));
        ImGui::PopStyleColor(2);
    }

    // ── Row 6: CPU / GPU / RAM footer ─────────────────────────────────────
    ImGui::Spacing();
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p.x, p.y),
            ImVec2(p.x + win_w, p.y),
            IM_COL32(100, 120, 100, 60), 1.0f);
        ImGui::Dummy(ImVec2(0, 3));
    }

    ImGui::TextColored(kLabel, "CPU ");
    ImGui::SameLine(0, 0);
    ImGui::Text("%.0f%%", snapshot.cpu_usage_percent);
    ImGui::SameLine(win_w * 0.38f);
    ImGui::TextColored(kLabel, "GPU ");
    ImGui::SameLine(0, 0);
    ImGui::Text("%.0f%%", snapshot.gpu_usage_percent);
    ImGui::SameLine(win_w * 0.72f);
    ImGui::TextColored(kLabel, "RAM ");
    ImGui::SameLine(0, 0);
    {
        const float ram_gb = snapshot.ram_used_mb / 1024.0f;
        if (ram_gb >= 1.0f)
            ImGui::Text("%.1fG", ram_gb);
        else
            ImGui::Text("%.0fM", snapshot.ram_used_mb);
    }

    EndWidget(layout);
}

} // namespace overlay::Widgets
