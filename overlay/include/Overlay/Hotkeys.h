#pragma once

#include <windows.h>

namespace overlay {

class Hotkeys final {
public:
    void Update();

    bool ConsumeToggleOverlay();
    bool ConsumeToggleSettings();
    bool ConsumePerformanceMode();

private:
    static bool Pressed(int virtual_key, SHORT& previous_state);

    SHORT insert_state_{0};
    SHORT f10_state_{0};
    SHORT f11_state_{0};

    bool toggle_overlay_{false};
    bool toggle_settings_{false};
    bool performance_mode_{false};
};

} // namespace overlay

