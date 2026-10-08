#include "Overlay/Hotkeys.h"

namespace overlay {

void Hotkeys::Update() {
    toggle_overlay_ = Pressed(VK_INSERT, insert_state_);
    toggle_settings_ = Pressed(VK_F10, f10_state_);
    performance_mode_ = Pressed(VK_F11, f11_state_);
}

bool Hotkeys::ConsumeToggleOverlay() {
    const bool result = toggle_overlay_;
    toggle_overlay_ = false;
    return result;
}

bool Hotkeys::ConsumeToggleSettings() {
    const bool result = toggle_settings_;
    toggle_settings_ = false;
    return result;
}

bool Hotkeys::ConsumePerformanceMode() {
    const bool result = performance_mode_;
    performance_mode_ = false;
    return result;
}

bool Hotkeys::Pressed(int virtual_key, SHORT& previous_state) {
    const SHORT state = GetAsyncKeyState(virtual_key);
    const bool was_down = (previous_state & 0x8000) != 0;
    const bool is_down = (state & 0x8000) != 0;
    previous_state = state;
    return is_down && !was_down;
}

} // namespace overlay

